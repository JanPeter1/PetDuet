<?php
/**
 * CYD Tamagotchi - Community Leaderboard Endpoint
 */
header('Content-Type: application/json');
header('Access-Control-Allow-Origin: *');

$db_file = __DIR__ . '/cyd_devices.json';
$devices = file_exists($db_file) ? json_decode(file_get_contents($db_file), true) : [];
if (!is_array($devices)) $devices = [];

$rankings = [];
foreach ($devices as $id => $d) {
    $rankings[] = [
        'id'               => 'lb_' . $id,
        'deviceId'         => $id,
        'petName'          => $d['petName'] ?? 'Pet',
        'trainerName'      => $d['deviceName'] ?? 'Trainer',
        'biome'            => $d['biome'] ?? 'grassland',
        'phase'            => $d['phase'] ?? 'Baby',
        'level'            => (int)($d['level'] ?? 1),
        'xp'               => (int)($d['xp'] ?? ($d['level'] * 200)),
        'recipesCrafted'   => (int)($d['recipesCrafted'] ?? 1),
        'jobsCompleted'    => (int)($d['jobsCompleted'] ?? 1),
        'spellsCast'       => (int)($d['spellsCast'] ?? 1),
        'friendTreatsSent' => (int)($d['friendTreatsSent'] ?? 0),
        'updatedAt'        => $d['lastSeen'] ?? time()
    ];
}

usort($rankings, function($a, $b) {
    if ($b['level'] !== $a['level']) {
        return $b['level'] - $a['level'];
    }
    return $b['xp'] - $a['xp'];
});

echo json_encode([
    'status' => 'success',
    'leaderboard' => $rankings
]);
