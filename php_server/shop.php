<?php
/**
 * PetDuet Shop & Marketplace Endpoint (Bazar)
 * ----------------------------------------------------------------------------
 * Serves the active item, recipe, and spell catalog loaded from rules.json.
 * Validates and processes purchases made by ESP32 devices or web companions.
 */
header('Content-Type: application/json; charset=UTF-8');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(200);
    exit;
}

// Load shop rules from rules.json
$rules_file = __DIR__ . '/rules.json';
if (!file_exists($rules_file)) {
    $rules_file = __DIR__ . '/../esp32_firmware/rules.json';
}

$shop_catalog = [
    'items' => [
        [
            'id' => 'food_apple',
            'name' => 'Crisp Apple',
            'name_de' => 'Frischer Apfel',
            'price_coins' => 5,
            'cost' => 5,
            'min_level' => 1,
            'category' => 'food',
            'icon' => '🍎',
            'description_de' => 'Knackiger roter Apfel (+12 Hunger, +5 Glueck).',
            'effects' => ['hunger' => 12, 'happiness' => 5, 'health' => 2, 'satiety_ticks' => 0, 'sick_chance' => 0]
        ],
        [
            'id' => 'food_snack',
            'name' => 'Sweet Pixel Snack',
            'name_de' => 'Suesser Pixel-Snack',
            'price_coins' => 8,
            'cost' => 8,
            'min_level' => 1,
            'category' => 'food',
            'icon' => '🥨',
            'description_de' => 'Suesser Snack (+25 Hunger, +15 Glueck) — 30% Risiko auf Bauchweh!',
            'effects' => ['hunger' => 25, 'happiness' => 15, 'energy' => 5, 'satiety_ticks' => 0, 'sick_chance' => 0.30]
        ],
        [
            'id' => 'food_herb_salad',
            'name' => 'Meadow Herb Salad',
            'name_de' => 'Wiesenkraeuter-Salat',
            'price_coins' => 22,
            'cost' => 22,
            'min_level' => 1,
            'category' => 'food',
            'icon' => '🥗',
            'description_de' => 'Fertiggericht (+35 Hunger, +10 HP). Haelt 4 Ticks ohne Abzug!',
            'effects' => ['hunger' => 35, 'health' => 10, 'happiness' => 10, 'energy' => 10, 'satiety_ticks' => 4, 'sick_chance' => 0]
        ],
        [
            'id' => 'food_berry_pie',
            'name' => 'Forest Berry Tart',
            'name_de' => 'Waldbeeren-Tarte',
            'price_coins' => 40,
            'cost' => 40,
            'min_level' => 2,
            'category' => 'food',
            'icon' => '🥧',
            'description_de' => 'Gourmet-Tarte (+45 Hunger, +25 Glueck). Haelt 5 Ticks ohne Abzug!',
            'effects' => ['hunger' => 45, 'happiness' => 25, 'health' => 15, 'satiety_ticks' => 5, 'sick_chance' => 0]
        ],
        [
            'id' => 'item_salve',
            'name' => 'Healing Salve',
            'name_de' => 'Wohltuende Heilsalbe',
            'price_coins' => 25,
            'cost' => 25,
            'min_level' => 1,
            'category' => 'potion',
            'icon' => '🧴',
            'description_de' => 'Heilsalbe fuer +40 HP — heilt Bauchweh & Krankheiten sofort!',
            'effects' => ['health' => 40, 'cure_sickness' => true]
        ],
        [
            'id' => 'item_energy_elixir',
            'name' => 'Energy Tonic',
            'name_de' => 'Vitalitaets-Tonikum',
            'price_coins' => 35,
            'cost' => 35,
            'min_level' => 2,
            'category' => 'potion',
            'icon' => '🧪',
            'description_de' => 'Staerkendes Tonikum fuer 60 Energie und 15 Mana.',
            'effects' => ['energy' => 60, 'mana' => 15]
        ]
    ],
    'recipes' => [
        [
            'id' => 'rcp_salad',
            'name' => 'Meadow Herb Salad',
            'name_de' => 'Wiesenkraeuter-Salat',
            'price_coins' => 15,
            'cost' => 15,
            'min_level' => 1,
            'icon' => '🥗',
            'description_de' => 'Rezeptrolle zum Zubereiten von frischem Wiesensalat.'
        ],
        [
            'id' => 'rcp_berry_pie',
            'name' => 'Forest Berry Tart',
            'name_de' => 'Waldbeeren-Tarte',
            'price_coins' => 65,
            'cost' => 65,
            'min_level' => 2,
            'icon' => '🥧',
            'description_de' => 'Schaltet die Faehigkeit frei, Waldbeeren-Tarte in der Kueche zu backen.'
        ],
        [
            'id' => 'rcp_stew',
            'name' => 'Kelp & Mushroom Stew',
            'name_de' => 'Tang- & Pilzeintopf',
            'price_coins' => 110,
            'cost' => 110,
            'min_level' => 4,
            'icon' => '🍲',
            'description_de' => 'Meisterrezept fuer nahrhaften Eintopf mit 8 Ticks Saettigung.'
        ]
    ],
    'spells' => [
        [
            'id' => 'spl_cleanse',
            'name' => 'Purify',
            'name_de' => 'Laeuterung',
            'price_coins' => 40,
            'cost' => 40,
            'min_level' => 1,
            'mana_cost' => 15,
            'icon' => '🫧',
            'description_de' => 'Zauberbuch zur sofortigen Reinigung und Krankheitsheilung.'
        ],
        [
            'id' => 'spl_heal',
            'name' => 'Vitality Aura',
            'name_de' => 'Vitalitaets-Aura',
            'price_coins' => 70,
            'cost' => 70,
            'min_level' => 2,
            'mana_cost' => 20,
            'icon' => '💖',
            'description_de' => 'Zauberbuch fuer eine waermende Aura mit +25 HP.'
        ],
        [
            'id' => 'spl_astral_harmony',
            'name' => 'Dual Astral Harmony',
            'name_de' => 'Astrale Harmonie',
            'price_coins' => 260,
            'cost' => 260,
            'min_level' => 5,
            'mana_cost' => 45,
            'icon' => '🌟',
            'description_de' => 'Maechtiger Koop-Zauber, der beide verbundenen Pets heilt.'
        ]
    ]
];

// If rules.json exists, pull live shop section
if (file_exists($rules_file)) {
    $rules_data = json_decode(file_get_contents($rules_file), true);
    if (!empty($rules_data['shop'])) {
        $shop_catalog = $rules_data['shop'];
    }
}

// ----------------------------------------------------------------------------
// GET: Fetch catalog
// ----------------------------------------------------------------------------
if ($_SERVER['REQUEST_METHOD'] === 'GET') {
    echo json_encode([
        'status' => 'success',
        'catalog' => $shop_catalog,
        'item_count' => count($shop_catalog['items'] ?? []),
        'recipe_count' => count($shop_catalog['recipes'] ?? []),
        'spell_count' => count($shop_catalog['spells'] ?? [])
    ], JSON_PRETTY_PRINT | JSON_UNESCAPED_UNICODE);
    exit;
}

// ----------------------------------------------------------------------------
// POST: Process purchase
// ----------------------------------------------------------------------------
$input = json_decode(file_get_contents('php://input'), true);
if ($_SERVER['REQUEST_METHOD'] === 'POST' && $input) {
    $device_id     = preg_replace('/[^a-zA-Z0-9_-]/', '', $input['deviceId'] ?? '');
    $item_type     = $input['itemType'] ?? 'item'; // 'item', 'recipe', 'spell'
    $item_id       = $input['itemId'] ?? '';
    $current_coins = (int)($input['currentCoins'] ?? 0);
    $current_level = (int)($input['currentLevel'] ?? 1);

    if (empty($device_id) || empty($item_id)) {
        http_response_code(400);
        echo json_encode(['status' => 'error', 'message' => 'Missing deviceId or itemId']);
        exit;
    }

    // Locate product in catalog
    $found_product = null;
    $catalog_section = ($item_type === 'recipe') ? ($shop_catalog['recipes'] ?? []) :
                       (($item_type === 'spell') ? ($shop_catalog['spells'] ?? []) : ($shop_catalog['items'] ?? []));

    foreach ($catalog_section as $prod) {
        if ($prod['id'] === $item_id) {
            $found_product = $prod;
            break;
        }
    }

    if (!$found_product) {
        http_response_code(404);
        echo json_encode(['status' => 'error', 'message' => 'Product not found in shop catalog']);
        exit;
    }

    $cost      = (int)($found_product['cost'] ?? $found_product['price_coins'] ?? 0);
    $min_level = (int)($found_product['min_level'] ?? 1);

    // Validate level
    if ($current_level < $min_level) {
        http_response_code(403);
        echo json_encode([
            'status' => 'error',
            'code' => 'LEVEL_TOO_LOW',
            'message' => "Requires Level $min_level (Current: Level $current_level)",
            'requiredLevel' => $min_level
        ]);
        exit;
    }

    // Validate funds
    if ($current_coins < $cost) {
        http_response_code(402);
        echo json_encode([
            'status' => 'error',
            'code' => 'INSUFFICIENT_FUNDS',
            'message' => "Cost $cost coins exceeds balance of $current_coins coins",
            'requiredCoins' => $cost
        ]);
        exit;
    }

    $new_coins = $current_coins - $cost;

    // Record purchase in device purchases log
    $purchases_dir = __DIR__ . '/purchases';
    if (!is_dir($purchases_dir)) {
        mkdir($purchases_dir, 0755, true);
    }
    $history_file = $purchases_dir . '/' . $device_id . '_shop.json';
    $history = file_exists($history_file) ? json_decode(file_get_contents($history_file), true) : [];
    if (!is_array($history)) $history = [];

    $purchase_record = [
        'timestamp' => time(),
        'itemType'  => $item_type,
        'itemId'    => $item_id,
        'itemName'  => $found_product['name'] ?? $item_id,
        'cost'      => $cost,
        'newCoins'  => $new_coins
    ];
    $history[] = $purchase_record;
    file_put_contents($history_file, json_encode($history, JSON_PRETTY_PRINT));

    echo json_encode([
        'status'         => 'success',
        'message'        => 'Purchase completed successfully',
        'product'        => $found_product,
        'cost'           => $cost,
        'previousCoins'  => $current_coins,
        'remainingCoins' => $new_coins,
        'transaction'    => $purchase_record
    ], JSON_PRETTY_PRINT | JSON_UNESCAPED_UNICODE);
    exit;
}

http_response_code(405);
echo json_encode(['status' => 'error', 'message' => 'Method not allowed']);
