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
            'id' => 'star_cookie',
            'name' => 'Star Cookie',
            'name_de' => 'Sternenkeks',
            'cost' => 15,
            'min_level' => 1,
            'hunger_gain' => 30,
            'happiness_gain' => 15,
            'icon' => '🍪',
            'description_de' => 'Ein knuspriger Keks, der den Hunger stillt und Freude schenkt.'
        ],
        [
            'id' => 'super_potion',
            'name' => 'Super Potion',
            'name_de' => 'Heiltrank',
            'cost' => 30,
            'min_level' => 2,
            'health_gain' => 50,
            'happiness_gain' => 20,
            'icon' => '🧪',
            'description_de' => 'Stellt 50 Gesundheit wieder her und beruhigt das Haustier.'
        ],
        [
            'id' => 'golden_apple',
            'name' => 'Golden Apple',
            'name_de' => 'Goldapfel',
            'cost' => 60,
            'min_level' => 4,
            'hunger_gain' => 50,
            'happiness_gain' => 40,
            'health_gain' => 30,
            'icon' => '🍎',
            'description_de' => 'Ein mystischer Apfel, der maximale Saettigung und Vitalitaet verleiht.'
        ]
    ],
    'recipes' => [
        [
            'id' => 'recipe_berry_pie',
            'name' => 'Recipe: Berry Pie',
            'name_de' => 'Rezept: Beerenkuchen',
            'cost' => 45,
            'min_level' => 2,
            'recipe_id' => 'berry_pie',
            'icon' => '🥧',
            'description_de' => 'Schaltet die Faehigkeit frei, Beerenkuchen in der Kueche zu backen.'
        ],
        [
            'id' => 'recipe_dragon_stew',
            'name' => 'Recipe: Dragon Stew',
            'name_de' => 'Rezept: Dracheneintopf',
            'cost' => 80,
            'min_level' => 5,
            'recipe_id' => 'dragon_stew',
            'icon' => '🍲',
            'description_de' => 'Geheimes Meisterrezept fuer feurige Staerkung.'
        ]
    ],
    'spells' => [
        [
            'id' => 'spell_aura_spark',
            'name' => 'Spell: Aura Spark',
            'name_de' => 'Zauber: Aura-Funke',
            'cost' => 70,
            'min_level' => 3,
            'spell_id' => 'aura_spark',
            'mana_cost' => 25,
            'heal_power' => 35,
            'icon' => '✨',
            'description_de' => 'Ein Zauber zur schnellen Wiederbelebung erschoepfter Energie.'
        ],
        [
            'id' => 'spell_celestial_blessing',
            'name' => 'Spell: Celestial Blessing',
            'name_de' => 'Zauber: Himmelssegen',
            'cost' => 120,
            'min_level' => 6,
            'spell_id' => 'celestial_blessing',
            'mana_cost' => 45,
            'heal_power' => 70,
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

    $cost      = (int)($found_product['cost'] ?? 0);
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
