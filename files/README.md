# Futuristic Launcher 🚀

A custom cyberpunk-styled application launcher for Wayland/X11 that replaces wofi/rofi with a sleek, futuristic interface.

![Features](https://img.shields.io/badge/Style-Cyberpunk-blueviolet)
![License](https://img.shields.io/badge/License-MIT-green)

## Features ✨

- 🎨 **Cyberpunk UI** - Glowing neon borders, gradients, and smooth animations
- 🔍 **Real-time Search** - Filter applications as you type
- ⚡ **Fast Launch** - Instant application launching
- 🎯 **Keyboard Navigation** - Full keyboard control with arrow keys
- 📦 **Desktop File Support** - Automatically finds all installed applications
- 🌈 **Beautiful Theming** - Blue/cyan neon glow aesthetic matching your panel

## Preview

The launcher features:
- Dark translucent background with gradient effects
- Glowing cyan borders and accents
- Smooth hover and selection animations
- Clean, modern typography
- Box-shadow glow effects for that cyberpunk feel

## Requirements 📋

### Build Dependencies

You need **either** GTK4 or GTK3 installed:

**For Ubuntu/Debian:**
```bash
# GTK4 (recommended)
sudo apt install build-essential libgtk-4-dev

# OR GTK3
sudo apt install build-essential libgtk-3-dev
```

**For Arch Linux:**
```bash
# GTK4 (recommended)
sudo pacman -S base-devel gtk4

# OR GTK3
sudo pacman -S base-devel gtk3
```

**For Fedora:**
```bash
# GTK4 (recommended)
sudo dnf install gcc-c++ gtk4-devel

# OR GTK3
sudo dnf install gcc-c++ gtk3-devel
```

## Installation 🔧

### Method 1: Quick Install (Recommended)

```bash
# Build the launcher
make

# Install system-wide
make install

# Or just use the binary directly
./futuristic-launcher
```

### Method 2: Manual Build

```bash
# For GTK4
g++ futuristic-launcher.cpp -o futuristic-launcher `pkg-config --cflags --libs gtk4` -std=c++17

# For GTK3
g++ futuristic-launcher.cpp -o futuristic-launcher `pkg-config --cflags --libs gtk+-3.0` -std=c++17

# Install manually
sudo cp futuristic-launcher /usr/local/bin/
sudo chmod +x /usr/local/bin/futuristic-launcher
```

## Configuration with Wayfire Panel ⚙️

Update your Wayfire config file (`~/.config/wayfire.ini`):

```ini
[futuristic-panel]
launcher_cmd = futuristic-launcher
```

Or if you didn't install it system-wide:

```ini
[futuristic-panel]
launcher_cmd = /path/to/futuristic-launcher
```

Then reload Wayfire or restart your panel. Now clicking the launcher button will open your custom futuristic launcher!

## Usage 🎮

### Keyboard Shortcuts

- **Type** - Search for applications
- **↑/↓** - Navigate through results
- **Enter** - Launch selected application
- **Esc** - Close launcher

### Mouse

- **Click** - Select and launch application
- **Scroll** - Navigate through list

## Customization 🎨

Want to change the colors? Edit the `WINDOW_CSS` section in `futuristic-launcher.cpp`:

```cpp
// Main accent color (cyan/blue glow)
rgba(80, 200, 255, ...)  // Change these RGB values

// Background darkness
rgba(3, 3, 8, ...)       // Adjust for lighter/darker background

// Border glow intensity
box-shadow: 0 0 25px ... // Increase blur radius for more glow
```

Then rebuild:
```bash
make clean
make
make install
```

## Troubleshooting 🔍

### Build Errors

**"gtk4 not found" or "gtk+-3.0 not found"**
- Install GTK development packages (see Requirements)
- Make sure pkg-config is installed: `sudo apt install pkg-config`

**"filesystem not found"**
- Your compiler needs C++17 support
- Update GCC: `sudo apt install g++-9` or newer

### Runtime Issues

**"No applications showing up"**
- Check if .desktop files exist: `ls /usr/share/applications/`
- Make sure you have applications installed
- Try running from terminal to see error messages: `./futuristic-launcher`

**"Launcher doesn't open from panel"**
- Check Wayfire config has correct path
- Test manually: `futuristic-launcher` in terminal
- Check file permissions: `chmod +x /usr/local/bin/futuristic-launcher`

**"Colors look wrong"**
- GTK theme might override some styling
- Try setting `GTK_THEME=Adwaita:dark` environment variable

## Uninstall 🗑️

```bash
make uninstall
# Or manually:
sudo rm /usr/local/bin/futuristic-launcher
```

## Technical Details 🔧

- **Language:** C++17
- **GUI Framework:** GTK4 or GTK3
- **Desktop Integration:** Parses .desktop files from standard XDG directories
- **Styling:** CSS-based theming with custom cyberpunk aesthetic
- **Performance:** Lightweight, instant startup

## Application Discovery

The launcher automatically searches for applications in:
- `/usr/share/applications/`
- `/usr/local/share/applications/`
- `~/.local/share/applications/`

It parses standard .desktop files and extracts:
- Application name
- Executable command
- Description/comment
- Icon (for future icon support)
- Categories

## Future Enhancements 🚧

Potential features for future versions:
- [ ] Icon display support
- [ ] Fuzzy search algorithm
- [ ] Recent/frequently used apps
- [ ] Custom keybindings
- [ ] Plugin system
- [ ] Animated window entrance/exit
- [ ] Sound effects
- [ ] More color themes

## Contributing 🤝

Feel free to submit issues, fork the repository, and create pull requests for any improvements.

## License 📄

MIT License - Feel free to use, modify, and distribute.

## Credits 💫

Created to complement the Wayfire Futuristic Panel with a matching cyberpunk aesthetic.

---

**Enjoy your futuristic launcher!** 🎉

If you have any issues or suggestions, feel free to open an issue or modify the code to suit your needs.
