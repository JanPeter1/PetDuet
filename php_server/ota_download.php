<?php
/**
 * PetDuet ESP32 OTA File Streaming Pipeline
 * ----------------------------------------------------------------------------
 * Delivers either the raw compiled ESP32 firmware binary (.bin) for A/B slot
 * flashing, or the JSON game rules file (rules.json) for dynamic SPIFFS reload.
 */
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, HEAD, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type, Range, X-ESP32-STA-MAC');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(200);
    exit;
}

$type = $_GET['type'] ?? 'firmware';

if ($type === 'rules') {
    $rules_file = __DIR__ . '/rules.json';
    if (!file_exists($rules_file)) {
        // Fallback to esp32_firmware if available
        $rules_file = __DIR__ . '/../esp32_firmware/rules.json';
    }

    if (!file_exists($rules_file)) {
        http_response_code(404);
        header('Content-Type: application/json');
        echo json_encode(['status' => 'error', 'message' => 'Rules file not found']);
        exit;
    }

    $content = file_get_contents($rules_file);
    header('Content-Type: application/json; charset=UTF-8');
    header('Content-Length: ' . strlen($content));
    header('X-File-SHA256: ' . hash('sha256', $content));
    header('Cache-Control: no-cache, must-revalidate');

    if ($_SERVER['REQUEST_METHOD'] === 'HEAD') {
        exit;
    }

    echo $content;
    exit;
}

// ----------------------------------------------------------------------------
// Firmware Binary (.bin) Streaming
// ----------------------------------------------------------------------------
$fw_file = __DIR__ . '/cyd_tamagotchi.bin';

if (!file_exists($fw_file)) {
    http_response_code(404);
    header('Content-Type: application/json');
    echo json_encode(['status' => 'error', 'message' => 'Firmware binary not found']);
    exit;
}

$filesize = filesize($fw_file);
$sha256   = hash_file('sha256', $fw_file);

header('Content-Type: application/octet-stream');
header('Content-Disposition: attachment; filename="cyd_tamagotchi.bin"');
header('Content-Length: ' . $filesize);
header('X-ESP32-Firmware-SHA256: ' . $sha256);
header('Cache-Control: no-cache, no-store, must-revalidate');

if ($_SERVER['REQUEST_METHOD'] === 'HEAD') {
    exit;
}

// Stream binary file
$fp = fopen($fw_file, 'rb');
if ($fp) {
    while (!feof($fp)) {
        echo fread($fp, 8192);
        flush();
    }
    fclose($fp);
}
exit;
