# Futuristic Launcher - Visual Features

## What Makes It Futuristic? 🎨

### Color Scheme
```
Background: Deep space black with subtle blue gradient
  - Primary: rgba(3, 3, 8, 0.98) → rgba(8, 8, 15, 0.98)
  
Borders: Glowing cyan/blue neon
  - Normal: rgba(50, 150, 255, 0.8)
  - Focused: rgba(80, 200, 255, 0.9)
  
Text: Bright cyan-tinted white
  - Primary: rgba(180, 230, 255, 1.0)
  - Hover: rgba(220, 240, 255, 1.0)
  - Selected: rgba(255, 255, 255, 1.0)

Glow Effects: Multiple layers of box-shadows
  - Inner glow: 0 0 15px
  - Outer glow: 0 0 25px
  - Ambient glow: 0 0 50px
```

### Design Elements

#### Window
- Borderless with custom 2px glowing cyan border
- Rounded corners (12px radius)
- Dark translucent background
- 600x500px default size
- Centered on screen

#### Search Bar
- Prominent position at top
- Bold text with cyan tint
- Pulsing glow on focus
- Real-time search as you type

#### Application List
- Clean, minimal design
- Gradient backgrounds on each item
- Hover effects with cyan glow
- Selected item has strong blue gradient + glow
- Smooth scroll
- App name in large bold text
- App description in smaller dimmed text

#### Animations
- Smooth color transitions (0.2s ease)
- Glow intensity changes on hover
- Scrolling animations
- Selection highlight transitions

### Cyberpunk Elements

1. **Neon Glows** - Every border glows with cyan light
2. **Dark Atmosphere** - Deep space black backgrounds
3. **Gradients** - Subtle blue gradients throughout
4. **Sharp Contrasts** - Bright text on dark background
5. **Tech Feel** - Monospace font options supported
6. **Clean Lines** - Rounded corners with precise borders

### Comparison to Standard Launchers

```
┌─────────────────────┬──────────────────────┬──────────────────────┐
│      wofi/rofi      │      dmenu           │  Futuristic Launcher │
├─────────────────────┼──────────────────────┼──────────────────────┤
│ Simple flat design  │ Minimalist bar       │ Glowing cyberpunk UI │
│ Standard borders    │ No borders           │ Neon glow borders    │
│ System theme colors │ Basic colors         │ Custom cyan/blue     │
│ No animations       │ No animations        │ Smooth transitions   │
│ Good functionality  │ Keyboard-focused     │ Both + visual appeal │
└─────────────────────┴──────────────────────┴──────────────────────┘
```

## Perfect Match with Your Panel

The launcher is designed to match your Wayfire Futuristic Panel:

### Shared Design Language
- Same color palette (cyan/blue on dark background)
- Matching glow effects
- Consistent border styles
- Similar gradient patterns
- Complementary typography

### Visual Continuity
When you click the launcher button on your panel, the launcher window appears with the same aesthetic, creating a seamless, cohesive desktop environment.

## Customization Tips

Want different colors? Edit these in the CSS:

```cpp
// For RED cyberpunk theme:
rgba(255, 50, 80, ...)  // Instead of (50, 150, 255, ...)

// For GREEN matrix theme:
rgba(50, 255, 100, ...) // Instead of (50, 150, 255, ...)

// For PURPLE sci-fi theme:
rgba(180, 80, 255, ...) // Instead of (50, 150, 255, ...)

// For ORANGE tech theme:
rgba(255, 150, 50, ...) // Instead of (50, 150, 255, ...)
```

## Technical Features

- GTK4/GTK3 native rendering (hardware accelerated)
- CSS-based theming (easy to customize)
- Instant application discovery
- Fast search algorithm
- Keyboard-optimized navigation
- Mouse-friendly design
- Standard .desktop file support
- Clean, maintainable C++ code

---

This launcher transforms your application launching experience from functional to spectacular! 🚀
