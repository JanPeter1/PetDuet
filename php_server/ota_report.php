<?php
/**
 * PetDuet ESP32 OTA Update Status Report Endpoint
 * Logs OTA install success, SHA256 verification failures, or automatic rollbacks.
 */
header('Content-Type: application/json; charset=UTF-8');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(200);
    exit;
}

$log_file = __DIR__ . '/cyd_ota_reports.json';
$reports = file_exists($log_file) ? json_decode(file_get_contents($log_file), true) : [];
if (!is_array($reports)) $reports = [];

if ($_SERVER['REQUEST_METHOD'] === 'GET') {
    echo json_encode([
        'status' => 'success',
        'reports' => $reports
    ], JSON_PRETTY_PRINT);
    exit;
}

$input = json_decode(file_get_contents('php://input'), true);
if ($_SERVER['REQUEST_METHOD'] === 'POST' && $input) {
    $device_id = preg_replace('/[^a-zA-Z0-9_-]/', '', $input['deviceId'] ?? '');
    $event     = substr(preg_replace('/[^a-zA-Z0-9_.-]/', '', $input['event'] ?? 'unknown'), 0, 32);
    $version   = substr($input['version'] ?? '', 0, 24);
    $partition = substr($input['partition'] ?? '', 0, 24);

    if (empty($device_id)) {
        http_response_code(400);
        echo json_encode(['status' => 'error', 'message' => 'Missing deviceId']);
        exit;
    }

    $entry = [
        'deviceId'  => $device_id,
        'event'     => $event,
        'version'   => $version,
        'partition' => $partition,
        'detail'    => substr((string)($input['detail'] ?? ''), 0, 120),
        'timestamp' => time()
    ];

    $reports[] = $entry;
    if (count($reports) > 100) {
        array_shift($reports);
    }
    file_put_contents($log_file, json_encode($reports, JSON_PRETTY_PRINT));

    echo json_encode([
        'status' => 'success',
        'logged' => $entry
    ], JSON_PRETTY_PRINT);
    exit;
}

http_response_code(405);
echo json_encode(['status' => 'error', 'message' => 'Method not allowed']);
