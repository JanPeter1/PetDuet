#!/bin/bash
# PetDuet OTA Update Helper Script
# Usage: ./ota_update.sh <new_version> [notes]
# Example: ./ota_update.sh 2.1.2 "Fixed touch calibration"

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FIRMWARE_BIN="$PROJECT_ROOT/esp32_firmware/.pio/build/esp32-2432s028r/firmware.bin"
SERVER_BIN="$PROJECT_ROOT/php_server/cyd_tamagotchi.bin"
MANIFEST="$PROJECT_ROOT/php_server/ota_manifest.json"
CONFIG_H="$PROJECT_ROOT/esp32_firmware/config.h"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Parse arguments
NEW_VERSION="${1:-}"
NOTES="${2:-Firmware update}"

if [ -z "$NEW_VERSION" ]; then
    echo -e "${RED}Error: Version required${NC}"
    echo "Usage: $0 <version> [notes]"
    echo "Example: $0 2.1.2 \"Fixed touch calibration\""
    exit 1
fi

# Validate version format (semver)
if ! [[ "$NEW_VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo -e "${RED}Error: Invalid version format. Use semantic versioning (e.g., 2.1.2)${NC}"
    exit 1
fi

echo -e "${YELLOW}=== PetDuet OTA Update: $NEW_VERSION ===${NC}"
echo ""

# 1. Check firmware binary exists
if [ ! -f "$FIRMWARE_BIN" ]; then
    echo -e "${RED}Error: Firmware binary not found at $FIRMWARE_BIN${NC}"
    echo "Build the firmware first: cd esp32_firmware && pio run"
    exit 1
fi

# 2. Copy firmware to server
echo -e "${GREEN}[1/6] Copying firmware to PHP server...${NC}"
cp "$FIRMWARE_BIN" "$SERVER_BIN"

# 3. Calculate SHA256 and size
echo -e "${GREEN}[2/6] Calculating SHA256 and size...${NC}"
SHA256=$(sha256sum "$SERVER_BIN" | cut -d' ' -f1)
SIZE=$(stat -c%s "$SERVER_BIN" 2>/dev/null || stat -f%z "$SERVER_BIN")
DATE=$(date +%Y-%m-%d)

echo "  SHA256: $SHA256"
echo "  Size:   $SIZE bytes"

# 4. Update config.h
echo -e "${GREEN}[3/6] Updating config.h FIRMWARE_VERSION...${NC}"
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" ]]; then
    # Windows/Git Bash
    sed -i "s/#define FIRMWARE_VERSION        \".*\"/#define FIRMWARE_VERSION        \"$NEW_VERSION\"/" "$CONFIG_H"
else
    # Linux/macOS
    sed -i "s/#define FIRMWARE_VERSION        \".*\"/#define FIRMWARE_VERSION        \"$NEW_VERSION\"/" "$CONFIG_H"
fi

# 5. Update ota_manifest.json
echo -e "${GREEN}[4/6] Updating ota_manifest.json...${NC}"
# Create a temporary Python script for JSON manipulation
cat > /tmp/update_manifest.py << EOF
import json
import sys

with open("$MANIFEST", 'r') as f:
    manifest = json.load(f)

manifest['firmware']['latest_version'] = "$NEW_VERSION"
manifest['firmware']['file_size'] = $SIZE
manifest['firmware']['sha256'] = "$SHA256"
manifest['firmware']['release_date'] = "$DATE"
manifest['firmware']['release_notes_en'] = "$NOTES"
manifest['firmware']['release_notes_de'] = "$NOTES"

with open("$MANIFEST", 'w') as f:
    json.dump(manifest, f, indent=2)

print("Manifest updated successfully")
EOF
python3 /tmp/update_manifest.py

# 6. Verify
echo -e "${GREEN}[5/6] Verifying updates...${NC}"
grep "FIRMWARE_VERSION" "$CONFIG_H"
python3 -c "
import json
with open('$MANIFEST') as f:
    m = json.load(f)
print(f\"Manifest FW: {m['firmware']['latest_version']}\")
print(f\"Manifest SHA: {m['firmware']['sha256'][:16]}...\")
print(f\"Manifest Size: {m['firmware']['file_size']}\")
"

# 7. Build reminder
echo -e "${GREEN}[6/6] Done!${NC}"
echo ""
echo -e "${YELLOW}Next steps:${NC}"
echo "  1. Build & flash new firmware to CYD:"
echo "     cd esp32_firmware && pio run -t upload"
echo ""
echo "  2. Start PHP server:"
echo "     php -S localhost:8080 -t php_server"
echo ""
echo "  3. Trigger OTA on CYD (tap center bridge dot or wait 15min auto-check)"
echo ""
echo -e "${GREEN}Server will now offer FW $NEW_VERSION to devices running older versions${NC}"