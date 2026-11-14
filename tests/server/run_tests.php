<?php
/**
 * PetDuet PHP Server Unit & Endpoint Test Suite + Coverage Reporter
 * Runs all tests against an isolated copy of php_server/ via local PHP HTTP server.
 */

$projectRoot = realpath(__DIR__ . '/../..');
$sourceServerDir = $projectRoot . DIRECTORY_SEPARATOR . 'php_server';
$covOutDir = $projectRoot . DIRECTORY_SEPARATOR . 'reports' . DIRECTORY_SEPARATOR . 'coverage';

foreach ($argv as $arg) {
    if (str_starts_with($arg, '--cov-dir=')) {
        $covOutDir = substr($arg, strlen('--cov-dir='));
    }
}

if (!is_dir($covOutDir)) {
    mkdir($covOutDir, 0777, true);
}

// 1. Copy php_server into isolated temp directory so repo state stays clean
$tmpRoot = sys_get_temp_dir() . DIRECTORY_SEPARATOR . 'petduet_srv_test_' . getmypid();
$tmpServer = $tmpRoot . DIRECTORY_SEPARATOR . 'php_server';
mkdir($tmpServer, 0777, true);

function copyDirRecursive(string $src, string $dst): void {
    if (!is_dir($dst)) {
        mkdir($dst, 0777, true);
    }
    foreach (scandir($src) as $item) {
        if ($item === '.' || $item === '..') continue;
        // Skip runtime generated state folders
        if (in_array($item, ['saves', 'purchases', 'cyd_devices.json', 'cyd_mailbox.json'], true)) continue;
        $from = $src . DIRECTORY_SEPARATOR . $item;
        $to   = $dst . DIRECTORY_SEPARATOR . $item;
        if (is_dir($from)) {
            copyDirRecursive($from, $to);
        } else {
            copy($from, $to);
        }
    }
}

function removeDirRecursive(string $dir): void {
    if (!is_dir($dir)) return;
    foreach (scandir($dir) as $item) {
        if ($item === '.' || $item === '..') continue;
        $path = $dir . DIRECTORY_SEPARATOR . $item;
        is_dir($path) ? removeDirRecursive($path) : @unlink($path);
    }
    @rmdir($dir);
}

copyDirRecursive($sourceServerDir, $tmpServer);

$covLogFile = $tmpRoot . DIRECTORY_SEPARATOR . 'server_cov_raw.json';
$prependFile = __DIR__ . DIRECTORY_SEPARATOR . 'coverage_prepend.php';

// Find free TCP port
$sock = stream_socket_server('tcp://127.0.0.1:0', $errno, $errstr);
$addr = stream_socket_get_name($sock, false);
fclose($sock);
[, $port] = explode(':', $addr);
$baseUrl = "http://127.0.0.1:$port";

// Server management
$phpBin = PHP_BINARY;
$extraArgs = '';
if (extension_loaded('xdebug')) {
    $extraArgs = '-d xdebug.mode=coverage ';
} elseif (extension_loaded('pcov')) {
    $extraArgs = '-d pcov.enabled=1 -d pcov.directory=' . escapeshellarg($tmpServer) . ' ';
}

$cmd = escapeshellarg($phpBin) . ' ' . $extraArgs
     . '-d auto_prepend_file=' . escapeshellarg($prependFile) . ' '
     . '-S 127.0.0.1:' . $port . ' -t ' . escapeshellarg($tmpServer);

$descriptors = [
    0 => ['pipe', 'r'],
    1 => ['pipe', 'w'],
    2 => ['pipe', 'w'],
];

$env = getenv();
$env['PETDUET_COV_LOG'] = $covLogFile;

$procOptions = ['bypass_shell' => true];

$requestCount = 0;
$maxRequestsPerServer = 15; // PHP built-in server limit ~20-22 requests

function startServer(): array {
    global $cmd, $descriptors, $tmpServer, $env, $procOptions, $port, $baseUrl;
    $proc = proc_open($cmd, $descriptors, $pipes, $tmpServer, $env, $procOptions);
    if (!is_resource($proc)) {
        fwrite(STDERR, "Failed to start local PHP test server\n");
        return [null, null];
    }
    // Wait until server accepts connections
    $ready = false;
    for ($i = 0; $i < 40; $i++) {
        $fp = @fsockopen('127.0.0.1', (int)$port, $e1, $e2, 0.2);
        if ($fp) {
            fclose($fp);
            $ready = true;
            break;
        }
        usleep(50000);
    }
    if (!$ready) {
        proc_terminate($proc);
        @fclose($pipes[0]); @fclose($pipes[1]); @fclose($pipes[2]);
        return [null, null];
    }
    return [$proc, $pipes];
}

function stopServer($proc, $pipes): void {
    if (is_resource($proc)) {
        proc_terminate($proc);
        @fclose($pipes[0]);
        @fclose($pipes[1]);
        @fclose($pipes[2]);
        proc_close($proc);
    }
}

// Start initial server
[$proc, $pipes] = startServer();
if (!$proc) {
    removeDirRecursive($tmpRoot);
    exit(1);
}

$assertionsRun = 0;
$assertionsFailed = 0;
$testedEndpoints = [];

function check(bool $cond, string $label): void {
    global $assertionsRun, $assertionsFailed;
    $assertionsRun++;
    if (!$cond) {
        $assertionsFailed++;
        fwrite(STDERR, "[FAIL] $label\n");
    }
}

function httpReq(string $method, string $path, $body = null): array {
    global $baseUrl, $testedEndpoints, $proc, $pipes, $requestCount, $maxRequestsPerServer, $cmd, $descriptors, $tmpServer, $env, $procOptions, $port;
    $ep = strtok(ltrim($path, '/'), '?');
    if ($ep === '') $ep = 'index.php';
    $testedEndpoints[$ep] = true;

    $headers = "Accept: application/json\r\n";
    $content = '';
    if ($body !== null) {
        $content = is_string($body) ? $body : json_encode($body);
        $headers .= "Content-Type: application/json\r\nContent-Length: " . strlen($content) . "\r\n";
    }

    $ctx = stream_context_create([
        'http' => [
            'method' => $method,
            'header' => $headers,
            'content' => $content,
            'ignore_errors' => true,
            'timeout' => 5,
        ]
    ]);
    $raw = @file_get_contents($baseUrl . $path, false, $ctx);
    $status = 0;
    if (isset($http_response_header[0]) && preg_match('/\s(\d{3})\s/', $http_response_header[0], $m)) {
        $status = (int)$m[1];
    }
    $json = is_string($raw) ? json_decode($raw, true) : null;

    $requestCount++;
    // Restart server if approaching limit
    if ($requestCount >= $maxRequestsPerServer) {
        stopServer($proc, $pipes);
        [$proc, $pipes] = startServer();
        if (!$proc) {
            fwrite(STDERR, "Failed to restart server\n");
            exit(1);
        }
        $requestCount = 0;
    }

    return ['status' => $status, 'raw' => $raw, 'json' => $json];
}

try {
    // 1. Gateway index.php
    $res = httpReq('GET', '/index.php');
    check($res['status'] === 200 && str_contains((string)$res['raw'], 'PetDuet CYD Server Gateway'), 'index.php returns HTML gateway');

    // 2. Device registration (OPTIONS, validation error, valid POST, GET list)
    $res = httpReq('OPTIONS', '/register.php');
    check($res['status'] === 200, 'register.php OPTIONS returns 200');

    $res = httpReq('POST', '/register.php', ['deviceId' => '']);
    check($res['status'] === 400, 'register.php rejects empty deviceId');

    $res = httpReq('POST', '/register.php', [
        'deviceId' => 'CYD-DEV1',
        'deviceName' => 'TestTrainer',
        'petName' => 'Leafkin',
        'biome' => 'grassland',
        'level' => 5,
        'phase' => 'Youth',
        'ipAddress' => '192.168.1.50',
        'port' => 8888,
    ]);
    check($res['status'] === 200 && ($res['json']['status'] ?? '') === 'success', 'register.php registers CYD-DEV1');

    $res = httpReq('POST', '/register.php', [
        'deviceId' => 'CYD-DEV2',
        'deviceName' => 'BuddyTrainer',
        'petName' => 'Aquadon',
        'biome' => 'water',
        'level' => 3,
        'phase' => 'Baby',
    ]);
    check($res['status'] === 200, 'register.php registers CYD-DEV2');

    $res = httpReq('GET', '/register.php');
    check($res['status'] === 200 && count($res['json']['devices'] ?? []) === 2, 'register.php GET lists 2 devices');

    // 3. Friend discovery
    $res = httpReq('GET', '/find_friend.php');
    check($res['status'] === 400, 'find_friend.php rejects missing friendId');

    $res = httpReq('GET', '/find_friend.php?friendId=UNKNOWN_DEV');
    check($res['status'] === 404, 'find_friend.php returns 404 for unknown device');

    $res = httpReq('GET', '/find_friend.php?friendId=CYD-DEV2');
    check($res['status'] === 200 && ($res['json']['friend']['isOnline'] ?? false) === true, 'find_friend.php finds CYD-DEV2 online');

    // 4. Leaderboard
    $res = httpReq('GET', '/leaderboard.php');
    check($res['status'] === 200 && ($res['json']['leaderboard'][0]['deviceId'] ?? '') === 'CYD-DEV1', 'leaderboard.php sorts highest level first');

    // 5. Sync Relay Mailbox
    $res = httpReq('OPTIONS', '/sync.php');
    check($res['status'] === 200, 'sync.php OPTIONS returns 200');

    $res = httpReq('POST', '/sync.php', ['fromDeviceId' => '', 'toDeviceId' => '']);
    check($res['status'] === 400, 'sync.php rejects missing device IDs');

    $res = httpReq('POST', '/sync.php', [
        'fromDeviceId' => 'CYD-DEV1',
        'toDeviceId' => 'CYD-DEV2',
        'type' => 'CHAT',
        'payload' => ['text' => 'Hallo Freund!']
    ]);
    check($res['status'] === 200 && !empty($res['json']['messageId']), 'sync.php queues CHAT relay packet');

    $res = httpReq('GET', '/sync.php?deviceId=CYD-DEV2');
    check($res['status'] === 200 && count($res['json']['messages'] ?? []) === 1, 'sync.php delivers pending messages and clears mailbox');

    $res = httpReq('GET', '/sync.php');
    check($res['status'] === 400, 'sync.php rejects GET without deviceId');

    // 6. Cloud Save
    $res = httpReq('OPTIONS', '/cloud_save.php');
    check($res['status'] === 200, 'cloud_save.php OPTIONS returns 200');

    $res = httpReq('POST', '/cloud_save.php', ['deviceId' => '']);
    check($res['status'] === 400, 'cloud_save.php rejects missing payload');

    $res = httpReq('POST', '/cloud_save.php', [
        'deviceId' => 'CYD-DEV1',
        'petData' => ['name' => 'Leafkin', 'level' => 5, 'coins' => 120]
    ]);
    check($res['status'] === 200 && ($res['json']['status'] ?? '') === 'success', 'cloud_save.php stores cloud backup');

    $res = httpReq('GET', '/cloud_save.php?deviceId=CYD-DEV1');
    check($res['status'] === 200 && ($res['json']['petData']['coins'] ?? 0) === 120, 'cloud_save.php restores cloud backup');

    $res = httpReq('GET', '/cloud_save.php?deviceId=NOPE');
    check($res['status'] === 404, 'cloud_save.php returns 404 on unknown device');

    // 7. Shop Endpoint
    $res = httpReq('OPTIONS', '/shop.php');
    check($res['status'] === 200, 'shop.php OPTIONS returns 200');

    $res = httpReq('GET', '/shop.php');
    check($res['status'] === 200 && ($res['json']['item_count'] ?? 0) >= 4, 'shop.php returns catalog');

    $res = httpReq('POST', '/shop.php', ['deviceId' => '', 'itemId' => '']);
    check($res['status'] === 400, 'shop.php rejects missing params');

    $res = httpReq('POST', '/shop.php', ['deviceId' => 'CYD-DEV1', 'itemType' => 'item', 'itemId' => 'unknown_xyz', 'currentCoins' => 999, 'currentLevel' => 10]);
    check($res['status'] === 404, 'shop.php returns 404 for unknown product');

    $res = httpReq('POST', '/shop.php', ['deviceId' => 'CYD-DEV1', 'itemType' => 'item', 'itemId' => 'food_berry_pie', 'currentCoins' => 999, 'currentLevel' => 1]);
    check($res['status'] === 403, 'shop.php blocks product below minLevel');

    $res = httpReq('POST', '/shop.php', ['deviceId' => 'CYD-DEV1', 'itemType' => 'item', 'itemId' => 'food_apple', 'currentCoins' => 1, 'currentLevel' => 1]);
    check($res['status'] === 402, 'shop.php blocks purchase with insufficient coins');

    $res = httpReq('POST', '/shop.php', ['deviceId' => 'CYD-DEV1', 'itemType' => 'item', 'itemId' => 'food_apple', 'currentCoins' => 50, 'currentLevel' => 1]);
    check($res['status'] === 200 && ($res['json']['remainingCoins'] ?? 0) === 45, 'shop.php completes valid purchase');

    // 8. OTA Check & OTA Download
    $res = httpReq('OPTIONS', '/ota_check.php');
    check($res['status'] === 200, 'ota_check.php OPTIONS returns 200');

    $res = httpReq('GET', '/ota_check.php?device=CYD-DEV1&fw_version=1.0.0&rules_version=1.0.0');
    check($res['status'] === 200 && ($res['json']['fw_update_available'] ?? false) === true, 'ota_check.php detects newer firmware when client is 1.0.0');

    $manifestData = json_decode(file_get_contents($tmpServer . DIRECTORY_SEPARATOR . 'ota_manifest.json'), true);
    $latestFw = $manifestData['firmware']['latest_version'] ?? '9.9.9';
    $res = httpReq('GET', '/ota_check.php?device=CYD-DEV1&fw_version=' . urlencode($latestFw));
    check($res['status'] === 200 && ($res['json']['fw_update_available'] ?? true) === false, 'ota_check.php reports up-to-date when on latest version');

    $res = httpReq('GET', '/ota_download.php?type=rules');
    check($res['status'] === 200 && str_contains((string)$res['raw'], 'decay'), 'ota_download.php serves rules.json');

    $res = httpReq('HEAD', '/ota_download.php?type=rules');
    check($res['status'] === 200, 'ota_download.php HEAD rules returns 200');

    $res = httpReq('GET', '/ota_download.php?type=firmware');
    check($res['status'] === 200 && strlen((string)$res['raw']) > 0, 'ota_download.php serves firmware binary');

    $res = httpReq('HEAD', '/ota_download.php?type=firmware');
    check($res['status'] === 200, 'ota_download.php HEAD firmware returns 200');

    // Also run index.php again after devices exist so table branch executes
    $res = httpReq('GET', '/index.php');
    check($res['status'] === 200 && str_contains((string)$res['raw'], 'CYD-DEV1'), 'index.php renders registered devices table');

} finally {
    stopServer($proc, $pipes);
}

// Compute PHP server code coverage
$covMode = 'none';
$perFileCoverage = [];
$totalExecuted = 0;
$totalExecutable = 0;

if (file_exists($covLogFile)) {
    $covData = json_decode(file_get_contents($covLogFile), true);
    if (is_array($covData) && !empty($covData['files'])) {
        $covMode = $covData['mode'] ?? 'extension';
        $normTmp = str_replace('\\', '/', $tmpServer);
        foreach ($covData['files'] as $filePath => $lineMap) {
            if (!str_starts_with($filePath, $normTmp . '/')) continue;
            $rel = substr($filePath, strlen($normTmp) + 1);
            $exec = 0;
            $all  = 0;
            foreach ($lineMap as $flag) {
                if ($flag === 1) {
                    $exec++;
                    $all++;
                } elseif ($flag === -1) {
                    $all++;
                }
            }
            if ($all > 0) {
                $perFileCoverage[$rel] = [
                    'executed' => $exec,
                    'total' => $all,
                    'pct' => round(($exec / $all) * 100.0, 2),
                ];
                $totalExecuted += $exec;
                $totalExecutable += $all;
            }
        }
        ksort($perFileCoverage);
    }
}

$phpFiles = glob($tmpServer . DIRECTORY_SEPARATOR . '*.php') ?: [];
$endpointNames = array_map('basename', $phpFiles);
sort($endpointNames);
$testedCount = count($testedEndpoints);
$totalEndpoints = count($endpointNames);
$endpointPct = $totalEndpoints > 0 ? round(($testedCount / $totalEndpoints) * 100.0, 2) : 0.0;
$linePct = $totalExecutable > 0 ? round(($totalExecuted / $totalExecutable) * 100.0, 2) : null;

$report = [
    'assertions_run' => $assertionsRun,
    'assertions_failed' => $assertionsFailed,
    'coverage_driver' => $covMode,
    'line_coverage_pct' => $linePct,
    'executed_lines' => $totalExecuted,
    'executable_lines' => $totalExecutable,
    'endpoint_coverage_pct' => $endpointPct,
    'tested_endpoints' => $testedCount,
    'total_endpoints' => $totalEndpoints,
    'files' => $perFileCoverage,
];

file_put_contents($covOutDir . DIRECTORY_SEPARATOR . 'server_coverage.json', json_encode($report, JSON_PRETTY_PRINT));

$txtLines = [];
$txtLines[] = "PetDuet PHP Server Test & Coverage Report";
$txtLines[] = "=========================================";
$txtLines[] = "Assertions: $assertionsRun | Failed: $assertionsFailed";
$txtLines[] = "Endpoint Coverage: $endpointPct% ($testedCount/$totalEndpoints PHP files)";
if ($linePct !== null) {
    $txtLines[] = "Line Coverage ($covMode): $linePct% ($totalExecuted/$totalExecutable lines)";
    foreach ($perFileCoverage as $file => $st) {
        $txtLines[] = sprintf("  - %-20s %6.2f%% (%d/%d lines)", $file, $st['pct'], $st['executed'], $st['total']);
    }
} else {
    $txtLines[] = "Line Coverage: N/A (weder PCOV noch Xdebug in PHP geladen)";
    $txtLines[] = "Hinweis: PCOV oder Xdebug in php.ini aktivieren fuer zeilengenaue PHP-Coverage.";
}
$txtReport = implode(PHP_EOL, $txtLines) . PHP_EOL;
file_put_contents($covOutDir . DIRECTORY_SEPARATOR . 'server_coverage.txt', $txtReport);

echo $txtReport;

removeDirRecursive($tmpRoot);
exit($assertionsFailed === 0 ? 0 : 1);