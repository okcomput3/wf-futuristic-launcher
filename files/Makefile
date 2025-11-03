# Makefile for Futuristic Launcher

CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2
TARGET = futuristic-launcher

# Try GTK4 first, fallback to GTK3
GTK_VERSION := $(shell pkg-config --exists gtk4 2>/dev/null && echo "gtk4" || echo "gtk+-3.0")

# Check for layer shell - prefer gtk-layer-shell-0
LAYER_SHELL := $(shell pkg-config --exists gtk-layer-shell-0 2>/dev/null && echo "gtk-layer-shell-0" || echo "")

# Get compiler flags
GTK_CFLAGS := $(shell pkg-config --cflags $(GTK_VERSION) $(LAYER_SHELL) 2>/dev/null || echo "")
GTK_LIBS := $(shell pkg-config --libs $(GTK_VERSION) $(LAYER_SHELL) 2>/dev/null || echo "")

# Fallback if pkg-config fails
ifeq ($(GTK_CFLAGS),)
    $(error pkg-config failed for $(GTK_VERSION). Please install libgtk-4-dev or libgtk-3-dev)
endif

all: $(TARGET)

$(TARGET): futuristic-launcher.cpp
	@echo "Building with $(GTK_VERSION)..."
	@if [ -n "$(LAYER_SHELL)" ]; then \
		echo "✓ Layer shell support: $(LAYER_SHELL)"; \
	else \
		echo "⚠ Layer shell not found - building without it"; \
		echo "  (Launcher will work but use centered positioning)"; \
	fi
	@echo "CFLAGS: $(GTK_CFLAGS)"
	@echo "LIBS: $(GTK_LIBS)"
	$(CXX) $(CXXFLAGS) $< -o $@ $(GTK_CFLAGS) $(GTK_LIBS)
	@echo "✓ Build successful! Binary: ./$(TARGET)"

install: $(TARGET)
	@echo "Installing to /usr/local/bin/..."
	sudo cp $(TARGET) /usr/local/bin/
	sudo chmod +x /usr/local/bin/$(TARGET)
	@echo "✓ Installation complete!"
	@echo ""
	@echo "Usage: futuristic-launcher"
	@echo "Configure in wayfire.ini: launcher_cmd = futuristic-launcher"

clean:
	rm -f $(TARGET)

uninstall:
	sudo rm -f /usr/local/bin/$(TARGET)
	@echo "✓ Uninstalled"

.PHONY: all install clean uninstall
