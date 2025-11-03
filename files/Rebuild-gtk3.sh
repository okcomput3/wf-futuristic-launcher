#!/bin/bash
# Build with GTK3 instead of GTK4 to avoid library conflicts
cd ~/Desktop/wayfire-launcher/files
echo "Building futuristic-launcher with GTK3..."
echo "(This avoids GTK2/3/4 mixing issues)"
echo ""
g++ -std=c++17 -Wall -O2 futuristic-launcher.cpp -o futuristic-launcher \
    $(pkg-config --cflags --libs gtk+-3.0 gtk-layer-shell-0 epoxy)
if [ $? -eq 0 ]; then
    echo ""
    echo "✓ Build successful!"
    echo ""
    echo "Installing..."
    sudo cp futuristic-launcher /usr/local/bin/
    sudo chmod +x /usr/local/bin/futuristic-launcher
    echo "✓ Installed to /usr/local/bin/futuristic-launcher"
    echo ""
    echo "═══════════════════════════════════════════════"
    echo "✓ INSTALLATION COMPLETE!"
    echo "═══════════════════════════════════════════════"
    echo ""
    echo "Now configured with GTK3 (no library conflicts)"
    echo ""
    echo "Next steps:"
    echo "1. Edit ~/.config/wayfire.ini"
    echo "   [futuristic-panel]"
    echo "   launcher_cmd = futuristic-launcher"
    echo ""
    echo "2. Reload Wayfire"
    echo "3. Click your launcher button!"
    echo ""
else
    echo "✗ Build failed"
    exit 1
fi
