<?php
/**
 * CYD Tamagotchi PHP Server Landing & Diagnostic Page
 */
header('Content-Type: text/html; charset=UTF-8');

$db_file = __DIR__ . '/cyd_devices.json';
$devices = file_exists($db_file) ? json_decode(file_get_contents($db_file), true) : [];
if (!is_array($devices)) $devices = [];
?>
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>CYD Tamagotchi Server Gateway</title>
  <style>
    body { font-family: monospace; background: #0f172a; color: #f8fafc; padding: 2rem; }
    h1 { color: #f59e0b; margin-bottom: 0.5rem; }
    .card { background: #1e293b; border: 1px solid #334155; border-radius: 8px; padding: 1.5rem; margin-top: 1rem; }
    table { width: 100%; border-collapse: collapse; margin-top: 1rem; }
    th, td { text-align: left; padding: 8px; border-bottom: 1px solid #334155; }
    th { color: #94a3b8; }
    .badge { background: #059669; color: #fff; padding: 2px 6px; border-radius: 4px; font-size: 11px; }
  </style>
</head>
<body>
  <h1>CYD Tamagotchi PHP Gateway</h1>
  <p>Hardware Target: ESP32 Cheap Yellow Display (ESP32-2432S028R / ESP_CoGo)</p>

  <div class="card">
    <h3>Registered Active CYD Devices (<?php echo count($devices); ?>)</h3>
    <table>
      <thead>
        <tr>
          <th>Device ID</th>
          <th>Trainer</th>
          <th>Pet Name</th>
          <th>Biome</th>
          <th>Level</th>
          <th>IP Address</th>
          <th>Status</th>
        </tr>
      </thead>
      <tbody>
        <?php if (empty($devices)): ?>
          <tr><td colspan="7" style="color: #64748b;">No devices currently registered. Boot your CYD to connect!</td></tr>
        <?php else: ?>
          <?php foreach ($devices as $d): ?>
            <tr>
              <td><strong><?php echo htmlspecialchars($d['deviceId'] ?? ''); ?></strong></td>
              <td><?php echo htmlspecialchars($d['deviceName'] ?? ''); ?></td>
              <td><?php echo htmlspecialchars($d['petName'] ?? ''); ?></td>
              <td><?php echo htmlspecialchars($d['biome'] ?? ''); ?></td>
              <td>Lv.<?php echo (int)($d['level'] ?? 1); ?></td>
              <td><code><?php echo htmlspecialchars($d['ipAddress'] ?? ''); ?>:<?php echo (int)($d['port'] ?? 8888); ?></code></td>
              <td><span class="badge">ONLINE</span></td>
            </tr>
          <?php endforeach; ?>
        <?php endif; ?>
      </tbody>
    </table>
  </div>
</body>
</html>
