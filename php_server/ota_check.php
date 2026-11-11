<?php
/**
 * PetDuet ESP32 OTA Update Check Service
 * ----------------------------------------------------------------------------
 * Handles version comparison for both binary firmware (A/B slot update)
 * and dynamic game rule files (rules.json SPIFFS hot-reload).
 */
header('Content-Type: application/json; charset=UTF-8');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type, X-ESP32-STA-MAC, X-ESP32-VERSION');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(200);
    exit;
}

$manifest_file = __DIR__ . '/ota_manifest.json';
$manifest = file_exists($manifest_file) ? json_decode(file_get_contents($manifest_file), true) : [];

$input = json_decode(file_get_contents('php://input'), true) ?? [];

$device_id     = $_GET['device']        ?? ($input['deviceId'] ?? 'CYD-DEV1');
$client_fw     = $_GET['fw_version']    ?? ($input['fwVersion'] ?? '1.0.0');
$client_rules  = $_GET['rules_version'] ?? ($input['rulesVersion'] ?? '1.0.0');

// Manifest details
$fw_manifest    = $manifest['firmware'] ?? [
    'latest_version' => '2.1.0',
    'download_url'   => 'ota_download.php?type=firmware',
    'file_size'      => 1428528,
    'sha256'         => '8f4e2c91b5a76e330d4198bfac851493e82012ef902b78129487c0e0b3d64112'
];

$rules_manifest = $manifest['rules'] ?? [
    'latest_version' => '1.0.4',
    'download_url'   => 'ota_download.php?type=rules',
    'file_size'      => 4210,
    'sha256'         => '3a7b51f089cbde7e62a39281a1795b2d87e0fa0c88344f62e879008bc215018e'
];

// Semver comparison helper
function is_newer_version($latest, $current) {
    return version_compare($latest, $current, '>');
}

$fw_available    = is_newer_version($fw_manifest['latest_version'], $client_fw);
$rules_available = is_newer_version($rules_manifest['latest_version'], $client_rules);

$response = [
    'status'                 => 'success',
    'device_id'              => $device_id,
    'timestamp'              => time(),
    'client_versions'        => [
        'firmware' => $client_fw,
        'rules'    => $client_rules
    ],
    // Binary Firmware OTA (A/B Dual Partition Switching)
    'fw_update_available'    => $fw_available,
    'fw_version'             => $fw_manifest['latest_version'],
    'fw_url'                 => $fw_manifest['download_url'],
    'fw_size'                => (int)$fw_manifest['file_size'],
    'fw_sha256'              => $fw_manifest['sha256'],
    'fw_notes'               => $fw_manifest['release_notes_de'] ?? 'Firmware Update',

    // Dynamic Rule File OTA (SPIFFS Hot-Reload without reboot)
    'rules_update_available' => $rules_available,
    'rules_version'          => $rules_manifest['latest_version'],
    'rules_url'              => $rules_manifest['download_url'],
    'rules_size'             => (int)$rules_manifest['file_size'],
    'rules_sha256'           => $rules_manifest['sha256'],
    'rules_notes'            => $rules_manifest['release_notes_de'] ?? 'Rules Update'
];

echo json_encode($response, JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES);
