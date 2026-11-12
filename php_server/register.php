<?php
/**
 * CYD Tamagotchi - Device Registration Endpoint
 * Compatible with ESP32-2432S028R (ESP_CoGo)
 */
header('Content-Type: application/json');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(200);
    exit;
}

$db_file = __DIR__ . '/cyd_devices.json';
$devices = file_exists($db_file) ? json_decode(file_get_contents($db_file), true) : [];
if (!is_array($devices)) $devices = [];

$input = json_decode(file_get_contents('php://input'), true);

if ($_SERVER['REQUEST_METHOD'] === 'POST' && $input) {
    $device_id = preg_replace('/[^a-zA-Z0-9_-]/', '', $input['deviceId'] ?? '');
    if (empty($device_id)) {
        http_response_code(400);
        echo json_encode(['status' => 'error', 'message' => 'Missing deviceId']);
        exit;
    }

    $client_ip = $_SERVER['REMOTE_ADDR'] ?? '127.0.0.1';
    $custom_ip = !empty($input['ipAddress']) ? $input['ipAddress'] : $client_ip;

    $devices[$device_id] = [
        'deviceId'    => $device_id,
        'deviceName'  => substr($input['deviceName'] ?? "CYD-" . substr($device_id, -4), 0, 32),
        'petName'     => substr($input['petName'] ?? 'Pet', 0, 32),
        'biome'       => $input['biome'] ?? 'grassland',
        'level'       => (int)($input['level'] ?? 1),
        'phase'       => $input['phase'] ?? 'Baby',
        'ipAddress'   => $custom_ip,
        'port'        => (int)($input['port'] ?? 8888),
        'lastSeen'    => time(),
        'stats'       => $input['stats'] ?? [
            'health' => 100, 'hunger' => 100, 'energy' => 100, 'happiness' => 100
        ]
    ];

    file_put_contents($db_file, json_encode($devices, JSON_PRETTY_PRINT));

    echo json_encode([
        'status' => 'success',
        'message' => 'CYD Device registered successfully',
        'device' => $devices[$device_id],
        'onlineCount' => count($devices)
    ]);
    exit;
}

// Return all active peers
echo json_encode([
    'status' => 'success',
    'devices' => array_values($devices)
]);
