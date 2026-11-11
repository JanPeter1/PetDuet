<?php
/**
 * PetDuet ESP32 CYD PHP Server Gateway & Diagnostics
 * ----------------------------------------------------------------------------
 * Hardware: ESP32-2432S028R "Cheap Yellow Display"
 * Services: Device Discovery, Dual-Slot A/B OTA, SPIFFS Rules, Shop & Cloud Save
 */
header('Content-Type: text/html; charset=UTF-8');

$devices_file = __DIR__ . '/cyd_devices.json';
$devices = file_exists($devices_file) ? json_decode(file_get_contents($devices_file), true) : [];
if (!is_array($devices)) $devices = [];

$manifest_file = __DIR__ . '/ota_manifest.json';
$manifest = file_exists($manifest_file) ? json_decode(file_get_contents($manifest_file), true) : [];
$fw_manifest = $manifest['firmware'] ?? ['latest_version' => '2.1.0'];
$rules_manifest = $manifest['rules'] ?? ['latest_version' => '1.0.4'];
?>
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>PetDuet CYD Server Gateway</title>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <style>
    body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, monospace; background: #0b1120; color: #f8fafc; padding: 2rem; margin: 0; line-height: 1.5; }
    h1 { color: #f59e0b; margin-top: 0; }
    .header-sub { color: #94a3b8; font-size: 0.95rem; margin-bottom: 2rem; }
    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(320px, 1fr)); gap: 1.5rem; margin-bottom: 2rem; }
    .card { background: #1e293b; border: 1px solid #334155; border-radius: 10px; padding: 1.5rem; box-shadow: 0 4px 6px -1px rgba(0,0,0,0.3); }
    .card h3 { color: #38bdf8; margin-top: 0; margin-bottom: 1rem; border-bottom: 1px solid #334155; padding-bottom: 0.5rem; }
    table { width: 100%; border-collapse: collapse; margin-top: 0.75rem; font-size: 0.9rem; }
    th, td { text-align: left; padding: 8px; border-bottom: 1px solid #334155; }
    th { color: #94a3b8; font-weight: 600; }
    .badge { background: #059669; color: #fff; padding: 2px 8px; border-radius: 4px; font-size: 11px; font-weight: 600; }
    .badge-ota { background: #7c3aed; color: #fff; padding: 2px 8px; border-radius: 4px; font-size: 11px; font-weight: 600; }
    .link-btn { display: inline-block; background: #2563eb; color: #fff; text-decoration: none; padding: 6px 12px; border-radius: 6px; font-size: 0.85rem; margin-top: 0.5rem; }
    .link-btn:hover { background: #1d4ed8; }
    code { background: #0f172a; padding: 2px 6px; border-radius: 4px; color: #fbbf24; font-size: 0.85rem; }
    ul { padding-left: 1.25rem; margin-bottom: 0; }
    li { margin-bottom: 0.4rem; }
  </style>
</head>
<body>
  <h1>🐾 PetDuet CYD Server Gateway</h1>
  <div class="header-sub">
    Hardware Target: <strong>ESP32-2432S028R</strong> (Cheap Yellow Display / ESP_CoGo) | Firmware v<?php echo htmlspecialchars($fw_manifest['latest_version']); ?> | Rules v<?php echo htmlspecialchars($rules_manifest['latest_version']); ?>
  </div>

  <div class="grid">
    <!-- Active OTA Management Card -->
    <div class="card">
      <h3>⚡ Over-the-Air (OTA) Distribution</h3>
      <p>Dual-slot A/B hardware partition switching & SPIFFS rules hot-reloading.</p>
      <ul>
        <li><strong>Firmware (A/B Slot):</strong> v<?php echo htmlspecialchars($fw_manifest['latest_version']); ?> <span class="badge-ota">Active</span></li>
        <li><strong>Dynamic Rules:</strong> v<?php echo htmlspecialchars($rules_manifest['latest_version']); ?> (Hot-Reload)</li>
        <li><strong>Check Endpoint:</strong> <code>/ota_check.php</code></li>
        <li><strong>Download Endpoint:</strong> <code>/ota_download.php</code></li>
      </ul>
      <div style="margin-top: 1rem;">
        <a class="link-btn" href="ota_check.php" target="_blank">Test OTA Check API</a>
        <a class="link-btn" href="ota_download.php?type=rules" target="_blank" style="background: #0d9488;">View Live rules.json</a>
      </div>
    </div>

    <!-- Shop & Marketplace Card -->
    <div class="card">
      <h3>🛍️ Shop & Marketplace System (Bazar)</h3>
      <p>Full item catalog, crafting recipes, and co-op spells with level & coin validation.</p>
      <ul>
        <li><strong>Endpoint:</strong> <code>/shop.php</code> (GET & POST)</li>
        <li><strong>Items:</strong> Star Cookie, Super Potion, Golden Apple</li>
        <li><strong>Crafting Recipes:</strong> Berry Pie, Dragon Stew</li>
        <li><strong>Co-op Spells:</strong> Aura Spark, Celestial Blessing</li>
      </ul>
      <div style="margin-top: 1rem;">
        <a class="link-btn" href="shop.php" target="_blank">View Shop Catalog JSON</a>
      </div>
    </div>

    <!-- Cloud Save & Relays Card -->
    <div class="card">
      <h3>💾 Cloud Save & P2P Discovery</h3>
      <p>Persistent device registration, cross-device relay fallback, and leaderboards.</p>
      <ul>
        <li><strong>Registration:</strong> <code>/register.php</code></li>
        <li><strong>Friend Discovery:</strong> <code>/find_friend.php</code></li>
        <li><strong>Cloud Save:</strong> <code>/cloud_save.php</code></li>
        <li><strong>P2P Relay:</strong> <code>/sync.php</code></li>
      </ul>
      <div style="margin-top: 1rem;">
        <a class="link-btn" href="register.php" target="_blank">List Registered Peers</a>
        <a class="link-btn" href="leaderboard.php" target="_blank" style="background: #d97706;">Leaderboard</a>
      </div>
    </div>
  </div>

  <!-- Connected Devices Table -->
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
          <th>Socket IP / Port</th>
          <th>Status</th>
        </tr>
      </thead>
      <tbody>
        <?php if (empty($devices)): ?>
          <tr><td colspan="7" style="color: #64748b; padding: 1.5rem 8px; text-align: center;">No CYD devices currently registered. Power on your CYD to establish automatic link!</td></tr>
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
