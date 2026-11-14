<?php
/**
 * CYD Tamagotchi - Remote Synchronization, Co-op Relay & Trade Escrow Endpoint
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
$escrow_file  = __DIR__ . '/cyd_trade_escrow.json';

$mailbox = file_exists($mailbox_file) ? json_decode(file_get_contents($mailbox_file), true) : [];
if (!is_array($mailbox)) $mailbox = [];

$escrows = file_exists($escrow_file) ? json_decode(file_get_contents($escrow_file), true) : [];
if (!is_array($escrows)) $escrows = [];

$input = json_decode(file_get_contents('php://input'), true);

if ($_SERVER['REQUEST_METHOD'] === 'POST' && $input) {
    $to_device   = preg_replace('/[^a-zA-Z0-9_-]/', '', $input['toDeviceId'] ?? '');
    $from_device = preg_replace('/[^a-zA-Z0-9_-]/', '', $input['fromDeviceId'] ?? '');
    $type        = substr($input['type'] ?? 'TREAT', 0, 32);
    $payload     = is_array($input['payload'] ?? null) ? $input['payload'] : [];

    if (empty($to_device) || empty($from_device)) {
        http_response_code(400);
        echo json_encode(['status' => 'error', 'message' => 'Missing toDeviceId or fromDeviceId']);
        exit;
    }

    if (!isset($mailbox[$to_device])) {
        $mailbox[$to_device] = [];
    }

    // Track 4-step Trade Escrow lifecycle on server when trade packets pass through relay
    $escrowStatus = null;
    $tradeId = preg_replace('/[^a-zA-Z0-9_-]/', '', (string)($payload['tradeId'] ?? ''));
    if ($type === 'TRADE_OFFER' && $tradeId !== '') {
        $escrows[$tradeId] = [
            'tradeId'       => $tradeId,
            'fromDeviceId'  => $from_device,
            'toDeviceId'    => $to_device,
            'offeredId'     => (string)($payload['offeredId'] ?? ''),
            'offeredName'   => (string)($payload['offeredName'] ?? ''),
            'requestedId'   => (string)($payload['requestedId'] ?? ''),
            'requestedName' => (string)($payload['requestedName'] ?? ''),
            'status'        => 'escrowed',
            'updatedAt'     => time(),
        ];
        $escrowStatus = 'escrowed';
        file_put_contents($escrow_file, json_encode($escrows, JSON_PRETTY_PRINT));
    } elseif ($type === 'TRADE_ACCEPT' && $tradeId !== '') {
        if (isset($escrows[$tradeId])) {
            $escrows[$tradeId]['status'] = 'completed';
            $escrows[$tradeId]['updatedAt'] = time();
            file_put_contents($escrow_file, json_encode($escrows, JSON_PRETTY_PRINT));
        }
        $escrowStatus = 'completed';
    } elseif (($type === 'TRADE_DECLINE' || $type === 'TRADE_CANCEL') && $tradeId !== '') {
        if (isset($escrows[$tradeId])) {
            $escrows[$tradeId]['status'] = ($type === 'TRADE_CANCEL') ? 'cancelled_refunded' : 'declined_refunded';
            $escrows[$tradeId]['updatedAt'] = time();
            file_put_contents($escrow_file, json_encode($escrows, JSON_PRETTY_PRINT));
        }
        $escrowStatus = ($type === 'TRADE_CANCEL') ? 'cancelled_refunded' : 'declined_refunded';
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

    $resp = [
        'status'    => 'success',
        'messageId' => $msg['id'],
        'timestamp' => $msg['timestamp']
    ];
    if ($escrowStatus !== null) {
        $resp['escrowStatus'] = $escrowStatus;
        $resp['tradeId'] = $tradeId;
    }
    echo json_encode($resp);
    exit;
}

// GET ?deviceId=... to poll pending messages (or ?escrow=1&deviceId=... to inspect escrows)
$poll_id = preg_replace('/[^a-zA-Z0-9_-]/', '', $_GET['deviceId'] ?? '');
if (empty($poll_id)) {
    http_response_code(400);
    echo json_encode(['status' => 'error', 'message' => 'Missing ?deviceId=...']);
    exit;
}

if (isset($_GET['escrow']) && $_GET['escrow'] === '1') {
    $deviceEscrows = [];
    foreach ($escrows as $rec) {
        if (($rec['fromDeviceId'] ?? '') === $poll_id || ($rec['toDeviceId'] ?? '') === $poll_id) {
            $deviceEscrows[] = $rec;
        }
    }
    echo json_encode([
        'status'  => 'success',
        'escrows' => $deviceEscrows,
    ]);
    exit;
}

$pending = $mailbox[$poll_id] ?? [];
$mailbox[$poll_id] = [];
file_put_contents($mailbox_file, json_encode($mailbox, JSON_PRETTY_PRINT));

echo json_encode([
    'status' => 'success',
    'messages' => $pending
]);
