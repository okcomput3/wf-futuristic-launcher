#!/bin/bash
# Futuristic Launcher - Installation Script

set -e

BLUE='\033[0;34m'
GREEN='\033[0;32m'
RED='\033[0;31m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

echo -e "${CYAN}"
echo "╔═══════════════════════════════════════╗"
echo "║   FUTURISTIC LAUNCHER INSTALLER       ║"
echo "║   Cyberpunk-styled App Launcher       ║"
echo "╚═══════════════════════════════════════╝"
echo -e "${NC}"

# Check for build dependencies
echo -e "${BLUE}[1/4]${NC} Checking dependencies..."

if ! command -v g++ &> /dev/null; then
    echo -e "${RED}Error: g++ not found. Please install build-essential or base-devel${NC}"
    exit 1
fi

if ! command -v pkg-config &> /dev/null; then
    echo -e "${RED}Error: pkg-config not found. Please install pkg-config${NC}"
    exit 1
fi

# Check for GTK
GTK_VERSION=""
LAYER_SHELL=""
if pkg-config --exists gtk4; then
    GTK_VERSION="gtk4"
    echo -e "${GREEN}✓ Found GTK4${NC}"
    if pkg-config --exists gtk4-layer-shell-0; then
        LAYER_SHELL="gtk4-layer-shell-0"
        echo -e "${GREEN}✓ Found GTK4 Layer Shell (Wayland positioning)${NC}"
    else
        echo -e "${CYAN}⚠ GTK4 Layer Shell not found - will use X11 fallback mode${NC}"
        echo -e "${CYAN}  For best experience install: libgtk-layer-shell${NC}"
    fi
elif pkg-config --exists gtk+-3.0; then
    GTK_VERSION="gtk+-3.0"
    echo -e "${GREEN}✓ Found GTK3${NC}"
    if pkg-config --exists gtk-layer-shell-0; then
        LAYER_SHELL="gtk-layer-shell-0"
        echo -e "${GREEN}✓ Found GTK Layer Shell (Wayland positioning)${NC}"
    else
        echo -e "${CYAN}⚠ GTK Layer Shell not found - will use X11 fallback mode${NC}"
        echo -e "${CYAN}  For best experience install: libgtk-layer-shell${NC}"
    fi
else
    echo -e "${RED}Error: Neither GTK4 nor GTK3 found.${NC}"
    echo "Please install GTK development packages:"
    echo "  Ubuntu/Debian: sudo apt install libgtk-4-dev libgtk-layer-shell-dev"
    echo "  Arch Linux: sudo pacman -S gtk4 gtk-layer-shell"
    echo "  Fedora: sudo dnf install gtk4-devel gtk-layer-shell-devel"
    exit 1
fi

# Build
echo -e "${BLUE}[2/4]${NC} Building futuristic-launcher..."
if make; then
    echo -e "${GREEN}✓ Build successful${NC}"
else
    echo -e "${RED}✗ Build failed${NC}"
    exit 1
fi

# Install
echo -e "${BLUE}[3/4]${NC} Installing to /usr/local/bin..."
if sudo cp futuristic-launcher /usr/local/bin/ && sudo chmod +x /usr/local/bin/futuristic-launcher; then
    echo -e "${GREEN}✓ Installation successful${NC}"
else
    echo -e "${RED}✗ Installation failed${NC}"
    exit 1
fi

# Configure Wayfire
echo -e "${BLUE}[4/4]${NC} Configuring Wayfire panel..."
WAYFIRE_CONFIG="$HOME/.config/wayfire.ini"

if [ -f "$WAYFIRE_CONFIG" ]; then
    # Backup config
    cp "$WAYFIRE_CONFIG" "$WAYFIRE_CONFIG.backup-$(date +%Y%m%d-%H%M%S)"
    
    # Check if launcher_cmd already exists
    if grep -q "launcher_cmd" "$WAYFIRE_CONFIG"; then
        echo -e "${CYAN}Found existing launcher_cmd in wayfire.ini${NC}"
        echo -e "${CYAN}Please manually update it to: launcher_cmd = futuristic-launcher${NC}"
    else
        # Try to add it to the futuristic-panel section
        if grep -q "\[futuristic-panel\]" "$WAYFIRE_CONFIG"; then
            sed -i '/\[futuristic-panel\]/a launcher_cmd = futuristic-launcher' "$WAYFIRE_CONFIG"
            echo -e "${GREEN}✓ Added launcher_cmd to wayfire.ini${NC}"
        else
            echo -e "${CYAN}Add this to your wayfire.ini:${NC}"
            echo -e "${CYAN}[futuristic-panel]${NC}"
            echo -e "${CYAN}launcher_cmd = futuristic-launcher${NC}"
        fi
    fi
else
    echo -e "${CYAN}Wayfire config not found at $WAYFIRE_CONFIG${NC}"
    echo -e "${CYAN}Add this to your wayfire.ini when you create it:${NC}"
    echo -e "${CYAN}[futuristic-panel]${NC}"
    echo -e "${CYAN}launcher_cmd = futuristic-launcher${NC}"
fi

echo ""
echo -e "${GREEN}╔═══════════════════════════════════════╗${NC}"
echo -e "${GREEN}║     INSTALLATION COMPLETE! 🚀         ║${NC}"
echo -e "${GREEN}╚═══════════════════════════════════════╝${NC}"
echo ""
echo -e "${CYAN}Next steps:${NC}"
echo -e "  1. Reload Wayfire (or log out and log back in)"
echo -e "  2. Click the launcher button on your panel"
echo -e "  3. Or test manually: ${GREEN}futuristic-launcher${NC}"
echo ""
echo -e "${CYAN}Keyboard shortcuts:${NC}"
echo -e "  • Type to search"
echo -e "  • ↑/↓ to navigate"
echo -e "  • Enter to launch"
echo -e "  • Esc to close"
echo ""
echo -e "${CYAN}To uninstall: ${NC}make uninstall"
echo ""
