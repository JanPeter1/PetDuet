<?php
/**
 * CYD Tamagotchi - Cloud Save & Restore
 */
header('Content-Type: application/json');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(200);
    exit;
}

$saves_dir = __DIR__ . '/saves';
if (!is_dir($saves_dir)) {
    mkdir($saves_dir, 0755, true);
}

$input = json_decode(file_get_contents('php://input'), true);

if ($_SERVER['REQUEST_METHOD'] === 'POST' && $input) {
    $device_id = preg_replace('/[^a-zA-Z0-9_-]/', '', $input['deviceId'] ?? '');
    $pet_data  = $input['petData'] ?? null;

    if (empty($device_id) || !$pet_data) {
        http_response_code(400);
        echo json_encode(['status' => 'error', 'message' => 'Missing deviceId or petData']);
        exit;
    }

    $save_payload = [
        'deviceId'  => $device_id,
        'petData'   => $pet_data,
        'updatedAt' => time()
    ];

    file_put_contents($saves_dir . '/' . $device_id . '.json', json_encode($save_payload, JSON_PRETTY_PRINT));

    echo json_encode([
        'status' => 'success',
        'message' => 'Pet progress saved to cloud',
        'updatedAt' => $save_payload['updatedAt']
    ]);
    exit;
}

// GET: Load save
$device_id = preg_replace('/[^a-zA-Z0-9_-]/', '', $_GET['deviceId'] ?? '');
$save_file = $saves_dir . '/' . $device_id . '.json';

if (!file_exists($save_file)) {
    http_response_code(404);
    echo json_encode(['status' => 'error', 'message' => 'No cloud save found for device ' . $device_id]);
    exit;
}

echo file_get_contents($save_file);
