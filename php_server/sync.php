<?php
/**
 * CYD Tamagotchi - Remote Synchronization & Relay Endpoint
 */
header('Content-Type: application/json');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(200);
    exit;
}

$mailbox_file = __DIR__ . '/cyd_mailbox.json';
$mailbox = file_exists($mailbox_file) ? json_decode(file_get_contents($mailbox_file), true) : [];
if (!is_array($mailbox)) $mailbox = [];

$input = json_decode(file_get_contents('php://input'), true);

if ($_SERVER['REQUEST_METHOD'] === 'POST' && $input) {
    $to_device   = preg_replace('/[^a-zA-Z0-9_-]/', '', $input['toDeviceId'] ?? '');
    $from_device = preg_replace('/[^a-zA-Z0-9_-]/', '', $input['fromDeviceId'] ?? '');
    $type        = substr($input['type'] ?? 'TREAT', 0, 32);
    $payload     = $input['payload'] ?? [];

    if (empty($to_device) || empty($from_device)) {
        http_response_code(400);
        echo json_encode(['status' => 'error', 'message' => 'Missing toDeviceId or fromDeviceId']);
        exit;
    }

    if (!isset($mailbox[$to_device])) {
        $mailbox[$to_device] = [];
    }

    $msg = [
        'id'           => uniqid('msg_', true),
        'fromDeviceId' => $from_device,
        'toDeviceId'   => $to_device,
        'type'         => $type,
        'payload'      => $payload,
        'timestamp'    => time()
    ];

    $mailbox[$to_device][] = $msg;
    if (count($mailbox[$to_device]) > 30) {
        array_shift($mailbox[$to_device]);
    }

    file_put_contents($mailbox_file, json_encode($mailbox, JSON_PRETTY_PRINT));

    echo json_encode([
        'status' => 'success',
        'messageId' => $msg['id'],
        'timestamp' => $msg['timestamp']
    ]);
    exit;
}

// GET ?deviceId=... to poll pending messages
$poll_id = preg_replace('/[^a-zA-Z0-9_-]/', '', $_GET['deviceId'] ?? '');
if (empty($poll_id)) {
    http_response_code(400);
    echo json_encode(['status' => 'error', 'message' => 'Missing ?deviceId=...']);
    exit;
}

$pending = $mailbox[$poll_id] ?? [];
$mailbox[$poll_id] = [];
file_put_contents($mailbox_file, json_encode($mailbox, JSON_PRETTY_PRINT));

echo json_encode([
    'status' => 'success',
    'messages' => $pending
]);
