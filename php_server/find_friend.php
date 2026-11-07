<?php
/**
 * CYD Tamagotchi - Friend Discovery Endpoint
 */
header('Content-Type: application/json');
header('Access-Control-Allow-Origin: *');

$db_file = __DIR__ . '/cyd_devices.json';
$devices = file_exists($db_file) ? json_decode(file_get_contents($db_file), true) : [];

$friend_id = preg_replace('/[^a-zA-Z0-9_-]/', '', $_GET['friendId'] ?? '');

if (empty($friend_id)) {
    http_response_code(400);
    echo json_encode(['status' => 'error', 'message' => 'Provide ?friendId=...']);
    exit;
}

if (!isset($devices[$friend_id])) {
    http_response_code(404);
    echo json_encode(['status' => 'error', 'message' => 'Friend device not found or offline']);
    exit;
}

$friend = $devices[$friend_id];
// Check if active within the last 10 minutes
$is_online = (time() - ($friend['lastSeen'] ?? 0)) < 600;

echo json_encode([
    'status' => 'success',
    'friend' => array_merge($friend, ['isOnline' => $is_online])
]);
