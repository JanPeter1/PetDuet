import express from 'express';
import { createServer as createViteServer } from 'vite';
import path from 'path';
import fs from 'fs';
import { fileURLToPath } from 'url';


const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const app = express();
const PORT = parseInt(process.env.PORT || '3000', 10);

app.use(express.json({ limit: '2mb' }));
app.use(express.static(path.resolve(__dirname, 'public')));
app.use('/assets', express.static(path.resolve(__dirname, 'assets')));

// In-memory store for registered CYD devices, cloud saves, leaderboards, and sync messages
interface DeviceRecord {
  deviceId: string;
  deviceName: string;
  petName: string;
  biome: string;
  level: number;
  phase: string;
  ipAddress: string;
  port: number;
  lastSeen: number;
  stats: {
    health: number;
    hunger: number;
    energy: number;
    happiness: number;
  };
}

interface CloudSaveRecord {
  deviceId: string;
  petData: any;
  updatedAt: number;
}

interface LeaderboardRecord {
  id: string;
  deviceId: string;
  petName: string;
  trainerName: string;
  biome: string;
  phase: string;
  level: number;
  xp: number;
  recipesCrafted: number;
  jobsCompleted: number;
  spellsCast: number;
  friendTreatsSent: number;
  updatedAt: number;
}

interface SyncMessage {
  id: string;
  fromDeviceId: string;
  toDeviceId: string;
  type: string;
  payload: any;
  timestamp: number;
}

const devices = new Map<string, DeviceRecord>();
const cloudSaves = new Map<string, CloudSaveRecord>();
const syncMailbox = new Map<string, SyncMessage[]>();

// Seed default leaderboards with charming retro CYD tamagotchi pioneers
const initialLeaderboard: LeaderboardRecord[] = [
  {
    id: 'lb-1',
    deviceId: 'CYD-842A',
    petName: 'Sunscale',
    trainerName: 'SolarHacker',
    biome: 'desert',
    phase: 'Adult',
    level: 42,
    xp: 8850,
    recipesCrafted: 35,
    jobsCompleted: 64,
    spellsCast: 92,
    friendTreatsSent: 48,
    updatedAt: Date.now() - 3600000 * 2,
  },
  {
    id: 'lb-2',
    deviceId: 'CYD-319F',
    petName: 'Bramble',
    trainerName: 'ForestSage',
    biome: 'forest',
    phase: 'Adult',
    level: 39,
    xp: 7920,
    recipesCrafted: 42,
    jobsCompleted: 58,
    spellsCast: 80,
    friendTreatsSent: 39,
    updatedAt: Date.now() - 3600000 * 5,
  },
  {
    id: 'lb-3',
    deviceId: 'CYD-654C',
    petName: 'Bubbles',
    trainerName: 'AquaMarine',
    biome: 'water',
    phase: 'Adult',
    level: 36,
    xp: 6840,
    recipesCrafted: 28,
    jobsCompleted: 45,
    spellsCast: 71,
    friendTreatsSent: 55,
    updatedAt: Date.now() - 3600000 * 8,
  },
  {
    id: 'lb-4',
    deviceId: 'CYD-102E',
    petName: 'Sprout',
    trainerName: 'MeadowRanger',
    biome: 'grassland',
    phase: 'Youth',
    level: 28,
    xp: 4320,
    recipesCrafted: 19,
    jobsCompleted: 33,
    spellsCast: 40,
    friendTreatsSent: 31,
    updatedAt: Date.now() - 3600000 * 12,
  },
];

const leaderboards = new Map<string, LeaderboardRecord>(
  initialLeaderboard.map((item) => [item.deviceId, item])
);

// --- API Endpoints matching PHP Server behavior ---

// 1. Device Registration (matches register.php)
app.post('/api/register', (req, res) => {
  const { deviceId, deviceName, petName, biome, level, phase, ipAddress, port, stats } = req.body;
  if (!deviceId) {
    return res.status(400).json({ error: 'Missing deviceId' });
  }

  const record: DeviceRecord = {
    deviceId,
    deviceName: deviceName || `CYD-${deviceId.slice(-4)}`,
    petName: petName || 'Unnamed Pet',
    biome: biome || 'grassland',
    level: level || 1,
    phase: phase || 'Baby',
    ipAddress: ipAddress || req.ip || '127.0.0.1',
    port: port || 8888,
    lastSeen: Date.now(),
    stats: stats || { health: 100, hunger: 100, energy: 100, happiness: 100 },
  };

  devices.set(deviceId, record);

  // Update leaderboard if higher level
  const existingLb = leaderboards.get(deviceId);
  if (!existingLb || (level && level > existingLb.level)) {
    leaderboards.set(deviceId, {
      id: `lb-${deviceId}`,
      deviceId,
      petName: record.petName,
      trainerName: record.deviceName,
      biome: record.biome,
      phase: record.phase,
      level: record.level,
      xp: (record.level - 1) * 200 + 50,
      recipesCrafted: existingLb ? existingLb.recipesCrafted : 1,
      jobsCompleted: existingLb ? existingLb.jobsCompleted : 1,
      spellsCast: existingLb ? existingLb.spellsCast : 1,
      friendTreatsSent: existingLb ? existingLb.friendTreatsSent : 0,
      updatedAt: Date.now(),
    });
  }

  res.json({
    status: 'success',
    message: 'Device registered successfully',
    device: record,
    onlineDevicesCount: devices.size,
  });
});

// 2. Peer Discovery (matches peers.php / find_friend.php)
app.get('/api/peers', (_req, res) => {
  const now = Date.now();
  // Filter devices seen in the last 15 minutes (or mock retain for demo)
  const peerList = Array.from(devices.values()).map((dev) => ({
    ...dev,
    isOnline: now - dev.lastSeen < 15 * 60 * 1000,
  }));
  res.json({
    status: 'success',
    peers: peerList,
  });
});

// 3. Find specific friend by device ID or friend code
app.get('/api/find_friend/:friendId', (req, res) => {
  const { friendId } = req.params;
  const friend = devices.get(friendId);
  if (!friend) {
    return res.status(404).json({ error: 'Friend device not found' });
  }
  res.json({
    status: 'success',
    friend,
  });
});

// 4. Send Message / Treat / Sync Action (matches sync.php)
app.post('/api/sync/send', (req, res) => {
  const { fromDeviceId, toDeviceId, type, payload } = req.body;
  if (!fromDeviceId || !toDeviceId || !type) {
    return res.status(400).json({ error: 'Missing required parameters' });
  }

  const message: SyncMessage = {
    id: `msg-${Date.now()}-${Math.random().toString(36).substr(2, 5)}`,
    fromDeviceId,
    toDeviceId,
    type,
    payload,
    timestamp: Date.now(),
  };

  const queue = syncMailbox.get(toDeviceId) || [];
  queue.push(message);
  // Keep last 50 messages max
  if (queue.length > 50) queue.shift();
  syncMailbox.set(toDeviceId, queue);

  // If action is treat, increment treats sent on leaderboard
  if (type === 'TREAT') {
    const lb = leaderboards.get(fromDeviceId);
    if (lb) {
      lb.friendTreatsSent += 1;
      leaderboards.set(fromDeviceId, lb);
    }
  }

  res.json({
    status: 'success',
    messageId: message.id,
    timestamp: message.timestamp,
  });
});

// 5. Poll messages for a device
app.get('/api/sync/poll/:deviceId', (req, res) => {
  const { deviceId } = req.params;
  const queue = syncMailbox.get(deviceId) || [];
  syncMailbox.set(deviceId, []); // Flush queue on poll
  res.json({
    status: 'success',
    messages: queue,
  });
});

// 6. Cloud Save (matches cloud_save.php)
app.post('/api/cloud_save', (req, res) => {
  const { deviceId, petData } = req.body;
  if (!deviceId || !petData) {
    return res.status(400).json({ error: 'Missing deviceId or petData' });
  }

  const saveRecord: CloudSaveRecord = {
    deviceId,
    petData,
    updatedAt: Date.now(),
  };
  cloudSaves.set(deviceId, saveRecord);

  // Update device record & leaderboard
  if (petData.level) {
    const existing = leaderboards.get(deviceId);
    leaderboards.set(deviceId, {
      id: `lb-${deviceId}`,
      deviceId,
      petName: petData.name || 'Pet',
      trainerName: petData.trainerName || `Trainer-${deviceId.slice(-4)}`,
      biome: petData.biome || 'grassland',
      phase: petData.phase || 'Baby',
      level: petData.level || 1,
      xp: petData.xp || 0,
      recipesCrafted: petData.recipesCraftedCount || (existing ? existing.recipesCrafted : 0),
      jobsCompleted: petData.jobsCompletedCount || (existing ? existing.jobsCompleted : 0),
      spellsCast: petData.spellsCastCount || (existing ? existing.spellsCast : 0),
      friendTreatsSent: petData.treatsSentCount || (existing ? existing.friendTreatsSent : 0),
      updatedAt: Date.now(),
    });
  }

  res.json({
    status: 'success',
    message: 'Pet data saved to cloud',
    updatedAt: saveRecord.updatedAt,
  });
});

app.get('/api/cloud_save/:deviceId', (req, res) => {
  const { deviceId } = req.params;
  const save = cloudSaves.get(deviceId);
  if (!save) {
    return res.status(404).json({ error: 'No cloud save found for this device' });
  }
  res.json({
    status: 'success',
    save,
  });
});

// 7. Global Leaderboards (matches leaderboard.php)
app.get('/api/leaderboard', (_req, res) => {
  const list = Array.from(leaderboards.values()).sort((a, b) => {
    if (b.level !== a.level) return b.level - a.level;
    return b.xp - a.xp;
  });
  res.json({
    status: 'success',
    leaderboard: list,
  });
});

// 8. Package Download Endpoints (ESP32 Firmware ZIP & PHP Server ZIP)
app.get('/api/download/firmware', (_req, res) => {
  const zipPath = path.resolve(__dirname, 'esp32_firmware.zip');
  res.download(zipPath, 'esp32_cyd_tamagotchi_firmware.zip', (err) => {
    if (err) {
      res.status(500).json({ error: 'Could not send firmware zip archive' });
    }
  });
});

app.get('/api/download/php-server', (_req, res) => {
  const zipPath = path.resolve(__dirname, 'php_server.zip');
  res.download(zipPath, 'cyd_tamagotchi_php_server.zip', (err) => {
    if (err) {
      res.status(500).json({ error: 'Could not send PHP server zip archive' });
    }
  });
});

// 9. Over-The-Air (OTA) A/B Firmware & Dynamic Rules Distribution Endpoints
app.get(['/api/ota/check', '/ota_check.php'], (req, res) => {
  const currentFw = (req.query.fw_version as string) || (req.query.version as string) || '1.0.0';
  const currentRules = (req.query.rules_version as string) || '1.0.0';
  const deviceId = (req.query.device as string) || 'CYD-DEV1';
  
  const fwUpdateAvailable = currentFw !== '2.1.0';
  const rulesUpdateAvailable = currentRules !== '1.0.4';

  res.json({
    status: 'success',
    device_id: deviceId,
    client_versions: {
      firmware: currentFw,
      rules: currentRules,
    },
    fw_update_available: fwUpdateAvailable,
    fw_version: '2.1.0',
    fw_url: '/ota_download.php?type=firmware',
    fw_size: 1428528,
    fw_sha256: '8f4e2c91b5a76e330d4198bfac851493e82012ef902b78129487c0e0b3d64112',
    fw_notes: 'Neues A/B OTA-System, Dual-Pet Stage & verbesserte Touch-Kalibrierung.',
    rules_update_available: rulesUpdateAvailable,
    rules_version: '1.0.4',
    rules_url: '/ota_download.php?type=rules',
    rules_size: 4210,
    rules_sha256: '3a7b51f089cbde7e62a39281a1795b2d87e0fa0c88344f62e879008bc215018e',
    rules_notes: 'Aktualisierte Shop-Items, Rezepte und Daily Missions.',
    firmware: {
      update_available: fwUpdateAvailable,
      target_version: '2.1.0',
      min_required_version: '1.0.0',
      url: '/ota_download.php?type=firmware',
      size_bytes: 1428528,
      sha256: '8f4e2c91b5a76e330d4198bfac851493e82012ef902b78129487c0e0b3d64112',
      release_notes_de: 'Neues A/B OTA-System, Dual-Pet Stage & verbesserte Touch-Kalibrierung.',
      release_notes_en: 'New A/B OTA system, dual-pet stage & improved touch calibration.'
    },
    rules: {
      update_available: rulesUpdateAvailable,
      target_version: '1.0.4',
      hash: '3a7b51f089cbde7e62a39281a1795b2d87e0fa0c88344f62e879008bc215018e',
      url: '/ota_download.php?type=rules',
      hot_reload: true
    }
  });
});

app.get(['/api/rules', '/ota_download.php'], (req, res) => {
  const type = (req.query.type as string) || '';
  if (type === 'firmware') {
    res.setHeader('Content-Type', 'application/octet-stream');
    res.setHeader('Content-Disposition', 'attachment; filename="cyd_tamagotchi.bin"');
    const dummyHeader = Buffer.from([0xe9, 0x04, 0x02, 0x20, 0x00, 0x00, 0x00, 0x40]);
    const dummyBody = Buffer.alloc(1024, 0xaa);
    return res.send(Buffer.concat([dummyHeader, dummyBody]));
  }

  const format = req.query.format as string;
  if (format === 'cfg') {
    const rulesPath = path.resolve(__dirname, 'src/engine/config/tamagotchi_rules.cfg');
    return res.sendFile(rulesPath);
  }
  const jsonPath = path.resolve(__dirname, 'src/engine/config/rules.json');
  res.setHeader('Content-Type', 'application/json');
  return res.sendFile(jsonPath);
});

// 10. Shop & Marketplace Endpoints
app.get(['/api/shop', '/shop.php'], (_req, res) => {
  const jsonPath = path.resolve(__dirname, 'src/engine/config/rules.json');
  try {
    const rulesData = JSON.parse(fs.readFileSync(jsonPath, 'utf-8'));
    res.json({
      status: 'success',
      catalog: rulesData.shop || {},
      item_count: (rulesData.shop?.items || []).length,
      recipe_count: (rulesData.shop?.recipes || []).length,
      spell_count: (rulesData.shop?.spells || []).length,
    });
  } catch (err: any) {
    res.status(500).json({ error: 'Failed to load shop catalog' });
  }
});

app.post(['/api/shop', '/shop.php'], (req, res) => {
  const { deviceId, itemType, itemId, currentCoins, currentLevel } = req.body;
  const jsonPath = path.resolve(__dirname, 'src/engine/config/rules.json');
  try {
    const rulesData = JSON.parse(fs.readFileSync(jsonPath, 'utf-8'));
    const section = itemType === 'recipe' ? (rulesData.shop?.recipes || []) :
                    itemType === 'spell' ? (rulesData.shop?.spells || []) : (rulesData.shop?.items || []);
    const product = section.find((p: any) => p.id === itemId);
    if (!product) {
      return res.status(404).json({ error: 'Product not found in shop catalog' });
    }
    const cost = product.cost || 0;
    const minLevel = product.min_level || 1;
    if ((currentLevel || 1) < minLevel) {
      return res.status(403).json({ error: `Requires level ${minLevel}` });
    }
    if ((currentCoins || 0) < cost) {
      return res.status(402).json({ error: 'Insufficient coins' });
    }
    res.json({
      status: 'success',
      product,
      cost,
      remainingCoins: currentCoins - cost,
      message: 'Purchase successful',
    });
  } catch (err: any) {
    res.status(500).json({ error: 'Shop transaction failed' });
  }
});

app.post('/api/ota/report', (req, res) => {
  const { deviceId, status, slot, error } = req.body;
  console.log(`[OTA REPORT] Device: ${deviceId}, Status: ${status}, Slot: ${slot}, Error: ${error || 'none'}`);
  res.json({ status: 'success', received: true });
});


// Setup Vite development middleware
async function startServer() {
  const vite = await createViteServer({
    server: { middlewareMode: true },
    appType: 'spa',
  });

  app.use(vite.middlewares);

  app.listen(PORT, '0.0.0.0', () => {
    console.log(`Tamagotchi CYD Server running on http://0.0.0.0:${PORT}`);
  });
}

startServer();
