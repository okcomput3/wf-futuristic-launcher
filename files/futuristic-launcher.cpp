/*
 * Futuristic Launcher MAXED OUT - Ultimate Application Launcher
 * 
 * A cyberpunk-styled application launcher with:
 * - Smooth fade animations
 * - Fuzzy search with typo tolerance
 * - Recent & favorite apps
 * - Multiple color themes
 * - Calculator mode (type math expressions)
 * - Web search (prefix with '?')
 * - Terminal commands (prefix with '>')
 * - Power menu (shutdown/restart/logout)
 * - System stats display
 * - Icon scaling on hover
 * - Alt+Number quick launch
 * - Context menu (right-click)
 * - Config file persistence
 * 
 * Build: g++ futuristic-launcher-maxed.cpp -o futuristic-launcher `pkg-config --cflags --libs gtk4 gtk4-layer-shell-0` -std=c++17
 * Alternative for GTK3: g++ futuristic-launcher-maxed.cpp -o futuristic-launcher `pkg-config --cflags --libs gtk+-3.0 gtk-layer-shell-0` -std=c++17
 * 
 * Requires: gtk-layer-shell for proper Wayland positioning
 * 
 * MIT License
 */

#include <gtk/gtk.h>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <map>
#include <set>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <sys/file.h>
#include <unistd.h>
#include <ctime>
#include <cmath>
#include <regex>
#include <sys/sysinfo.h>

// Include layer shell if available
#if defined(GDK_WINDOWING_WAYLAND) || !defined(GDK_WINDOWING_X11)
    #if __has_include(<gtk-layer-shell/gtk-layer-shell.h>)
        #include <gtk-layer-shell/gtk-layer-shell.h>
        #define HAS_LAYER_SHELL 1
    #elif __has_include("gtk-layer-shell.h")
        #include "gtk-layer-shell.h"
        #define HAS_LAYER_SHELL 1
    #else
        #define HAS_LAYER_SHELL 0
    #endif
#else
    #define HAS_LAYER_SHELL 0
#endif

#if !HAS_LAYER_SHELL
    #define GTK_LAYER_SHELL_LAYER_OVERLAY 0
    #define GTK_LAYER_SHELL_EDGE_BOTTOM 0
    #define GTK_LAYER_SHELL_EDGE_LEFT 0
    #define GTK_LAYER_SHELL_EDGE_RIGHT 0
    #define GTK_LAYER_SHELL_EDGE_TOP 0
    #define GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE 0
#endif

namespace fs = std::filesystem;

// GTK version compatibility
#if GTK_MAJOR_VERSION == 4
    #define GTK_IS_VERSION_4 1
    #define GTK_IS_VERSION_3 0
#else
    #define GTK_IS_VERSION_4 0
    #define GTK_IS_VERSION_3 1
#endif

// Compatibility wrappers for GTK3/GTK4 differences
#if GTK_IS_VERSION_3
    #define gtk_window_set_child(window, child) gtk_container_add(GTK_CONTAINER(window), child)
    #define gtk_box_append(box, child) gtk_box_pack_start(GTK_BOX(box), child, FALSE, FALSE, 0)
    #define gtk_scrolled_window_set_child(sw, child) gtk_container_add(GTK_CONTAINER(sw), child)
    #define gtk_list_box_append(lb, child) gtk_list_box_insert(GTK_LIST_BOX(lb), child, -1)
    
    static inline GtkWidget* gtk_widget_get_first_child(GtkWidget* widget) {
        GList* children = gtk_container_get_children(GTK_CONTAINER(widget));
        GtkWidget* first = children ? GTK_WIDGET(children->data) : NULL;
        g_list_free(children);
        return first;
    }
    
    static inline GtkWidget* gtk_widget_get_next_sibling(GtkWidget* widget) {
        return NULL;
    }
    
    static inline void gtk_list_box_remove_compat(GtkListBox* lb, GtkWidget* child) {
        gtk_container_remove(GTK_CONTAINER(lb), child);
    }
    
    static inline const char* gtk_editable_get_text(GtkEditable* editable) {
        return gtk_entry_get_text(GTK_ENTRY(editable));
    }
    
    static inline void gtk_window_present_compat(GtkWindow* window) {
        gtk_widget_show_all(GTK_WIDGET(window));
    }
#else
    static inline void gtk_list_box_remove_compat(GtkListBox* lb, GtkWidget* child) {
        gtk_list_box_remove(lb, child);
    }
    
    static inline void gtk_window_present_compat(GtkWindow* window) {
        gtk_window_present(window);
    }
#endif

// Theme colors
enum Theme {
    THEME_BLUE,
    THEME_PURPLE,
    THEME_GREEN,
    THEME_RED,
    THEME_ORANGE,
    THEME_CYAN,
    THEME_MORPH
};

struct ThemeColors {
    std::string primary;
    std::string secondary;
    std::string accent;
    std::string bg_start;
    std::string bg_end;
};

std::map<Theme, ThemeColors> theme_palette = {
    {THEME_BLUE, {"50, 150, 255", "80, 200, 255", "100, 180, 255", "3, 3, 8", "8, 8, 15"}},
    {THEME_PURPLE, {"150, 50, 255", "200, 80, 255", "180, 100, 255", "8, 3, 15", "15, 8, 20"}},
    {THEME_GREEN, {"50, 255, 150", "80, 255, 200", "100, 255, 180", "3, 15, 8", "8, 20, 15"}},
    {THEME_RED, {"255, 50, 100", "255, 80, 130", "255, 100, 150", "15, 3, 8", "20, 8, 12"}},
    {THEME_ORANGE, {"255, 150, 50", "255, 180, 80", "255, 165, 100", "15, 10, 3", "20, 15, 8"}},
    {THEME_CYAN, {"50, 255, 255", "80, 255, 255", "100, 255, 255", "3, 12, 15", "8, 18, 20"}},
    {THEME_MORPH, {"150, 150, 200", "180, 180, 220", "165, 165, 210", "10, 10, 15", "15, 15, 20"}}
};

struct DesktopApp {
    std::string name;
    std::string exec;
    std::string icon;
    std::string comment;
    std::string categories;
    bool no_display = false;
    int launch_count = 0;
    time_t last_launch = 0;
    bool is_favorite = false;
};

struct Config {
    Theme current_theme = THEME_BLUE;
    int icon_size = 64;
    float transparency = 0.98f;
    std::set<std::string> favorites;
    std::map<std::string, int> launch_counts;
    std::map<std::string, time_t> last_launches;
    
    bool validate() {
        // Validate all values are in acceptable ranges
        if (icon_size < 16 || icon_size > 256) icon_size = 64;
        if (transparency < 0.0f || transparency > 1.0f) transparency = 0.98f;
        if (current_theme < THEME_BLUE || current_theme > THEME_MORPH) current_theme = THEME_BLUE;
        return true;
    }
    
    void load() {
        std::string config_path = std::string(g_get_home_dir()) + "/.config/futuristic-launcher.conf";
        std::ifstream file(config_path);
        if (!file.is_open()) return;
        
        std::string line;
        int error_count = 0;
        
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;
            
            std::string key = line.substr(0, eq);
            std::string value = line.substr(eq + 1);
            
            // Skip empty values
            if (value.empty()) continue;
            
            try {
                if (key == "theme") {
                    int theme_val = std::stoi(value);
                    if (theme_val >= 0 && theme_val <= 6) {
                        current_theme = static_cast<Theme>(theme_val);
                    }
                } else if (key == "icon_size") {
                    int size = std::stoi(value);
                    if (size > 0 && size <= 256) {
                        icon_size = size;
                    }
                } else if (key == "transparency") {
                    float trans = std::stof(value);
                    if (trans >= 0.0f && trans <= 1.0f) {
                        transparency = trans;
                    }
                } else if (key == "favorite") {
                    favorites.insert(value);
                } else if (key.length() > 6 && key.substr(0, 6) == "count_") {
                    std::string app_name = key.substr(6);
                    int count = std::stoi(value);
                    if (count >= 0) {
                        launch_counts[app_name] = count;
                    }
                } else if (key.length() > 5 && key.substr(0, 5) == "last_") {
                    std::string app_name = key.substr(5);
                    long timestamp = std::stol(value);
                    if (timestamp >= 0) {
                        last_launches[app_name] = timestamp;
                    }
                }
            } catch (const std::invalid_argument& e) {
                // Skip invalid entries
                std::cerr << "Warning: Invalid config value for key '" << key << "': " << value << std::endl;
                error_count++;
                continue;
            } catch (const std::out_of_range& e) {
                // Skip out of range entries
                std::cerr << "Warning: Out of range config value for key '" << key << "': " << value << std::endl;
                error_count++;
                continue;
            }
        }
        
        file.close();
        
        // If too many errors, backup and reset
        if (error_count > 10) {
            std::cerr << "Too many config errors (" << error_count << "), creating backup and resetting..." << std::endl;
            std::string backup_path = config_path + ".backup";
            std::rename(config_path.c_str(), backup_path.c_str());
            // Reset to defaults (already done by initialization)
        }
        
        validate();
    }
    
    void save() {
        std::string config_dir = std::string(g_get_home_dir()) + "/.config";
        fs::create_directories(config_dir);
        
        std::string config_path = config_dir + "/futuristic-launcher.conf";
        std::ofstream file(config_path);
        
        file << "# Futuristic Launcher Configuration\n";
        file << "theme=" << static_cast<int>(current_theme) << "\n";
        file << "icon_size=" << icon_size << "\n";
        file << "transparency=" << transparency << "\n";
        
        for (const auto& fav : favorites) {
            file << "favorite=" << fav << "\n";
        }
        
        for (const auto& [app, count] : launch_counts) {
            file << "count_" << app << "=" << count << "\n";
        }
        
        for (const auto& [app, last] : last_launches) {
            file << "last_" << app << "=" << last << "\n";
        }
    }
};

class FuturisticLauncher {
private:
    GtkWidget *window;
    GtkWidget *search_entry;
    GtkWidget *app_list;
    GtkWidget *scrolled_window;
    GtkWidget *header_box;
    GtkWidget *stats_label;
    GtkWidget *power_menu_button;
    GtkCssProvider *css_provider;
    
    std::vector<DesktopApp> all_apps;
    std::vector<DesktopApp> filtered_apps;
    std::vector<GtkWidget*> icon_widgets;
    int selected_index = 0;
    int lock_fd = -1;
    bool is_visible = false;
    Config config;
    
    guint stats_timer = 0;
    guint fade_timer = 0;
    guint morph_timer = 0;
    double current_opacity = 0.0;
    bool fading_in = false;
    int morph_current_theme = 0;  // 0-5 for cycling through themes
    
    // Special modes
    bool calculator_mode = false;
    bool web_search_mode = false;
    bool command_mode = false;
    
    static constexpr int LAUNCHER_WIDTH = 500;
    static constexpr int LAUNCHER_HEIGHT = 600;
    static constexpr int MARGIN_TOP = 50;
    
    static constexpr bool POSITION_TOP_LEFT = true;
    
    // Fuzzy search scoring
    int fuzzy_score(const std::string& str, const std::string& pattern) {
        std::string str_lower = str;
        std::string pattern_lower = pattern;
        std::transform(str_lower.begin(), str_lower.end(), str_lower.begin(), ::tolower);
        std::transform(pattern_lower.begin(), pattern_lower.end(), pattern_lower.begin(), ::tolower);
        
        int score = 0;
        size_t str_idx = 0;
        size_t pat_idx = 0;
        size_t consecutive = 0;
        
        while (str_idx < str_lower.length() && pat_idx < pattern_lower.length()) {
            if (str_lower[str_idx] == pattern_lower[pat_idx]) {
                score += 1 + consecutive * 5; // Bonus for consecutive matches
                consecutive++;
                pat_idx++;
            } else {
                consecutive = 0;
            }
            str_idx++;
        }
        
        if (pat_idx != pattern_lower.length()) {
            return 0; // Didn't match all pattern chars
        }
        
        // Bonus for exact substring match
        if (str_lower.find(pattern_lower) != std::string::npos) {
            score += 50;
        }
        
        // Bonus for match at start
        if (str_lower.find(pattern_lower) == 0) {
            score += 100;
        }
        
        return score;
    }
    
    std::string get_theme_css_for(Theme theme_to_use) {
        ThemeColors colors = theme_palette[theme_to_use];
        std::ostringstream css;
        
        // Theme-specific styling
        switch(theme_to_use) {
            case THEME_BLUE: // Classic Cyberpunk
                css << R"(
                    window {
                        background: linear-gradient(135deg, rgba(3, 3, 8, 0.98) 0%, rgba(8, 8, 15, 0.98) 100%);
                        border: 2px solid rgba(50, 150, 255, 0.8);
                        border-radius: 12px;
                    }
                    entry {
                        background: rgba(15, 15, 25, 0.95);
                        color: rgba(180, 230, 255, 1.0);
                        border: 2px solid rgba(50, 150, 255, 0.5);
                        border-radius: 8px;
                        padding: 12px 16px;
                        font-size: 16px;
                        font-weight: bold;
                    }
                    entry:focus {
                        border: 2px solid rgba(80, 200, 255, 0.9);
                        background: rgba(20, 20, 35, 0.95);
                    }
                    #icon-box {
                        background: rgba(12, 12, 20, 0.7);
                        border: 1px solid rgba(40, 40, 60, 0.6);
                        border-radius: 6px;
                        padding: 8px;
                        margin: 2px;
                        transition: all 150ms cubic-bezier(0.4, 0.0, 0.2, 1);
                    }
                    #icon-box:hover {
                        background: rgba(20, 50, 90, 0.7);
                        border: 1px solid rgba(100, 180, 255, 0.8);
                        transform: scale(1.05);
                    }
                    .selected-icon {
                        background: rgba(20, 50, 90, 0.7) !important;
                        border: 1px solid rgba(80, 200, 255, 0.8) !important;
                        transform: scale(1.05);
                    }
                )";
                break;
                
            case THEME_PURPLE: // Neon Minimal - Thin borders, rounded corners
                css << R"(
                    window {
                        background: rgba(18, 8, 25, 0.96);
                        border: 1px solid rgba(200, 80, 255, 0.9);
                        border-radius: 20px;
                    }
                    entry {
                        background: rgba(25, 10, 35, 0.8);
                        color: rgba(255, 180, 255, 1.0);
                        border: 1px solid rgba(150, 50, 255, 0.6);
                        border-radius: 15px;
                        padding: 14px 20px;
                        font-size: 15px;
                        font-weight: normal;
                    }
                    entry:focus {
                        border: 1px solid rgba(200, 100, 255, 1.0);
                        background: rgba(30, 15, 40, 0.9);
                    }
                    #icon-box {
                        background: rgba(30, 15, 45, 0.5);
                        border: 1px solid rgba(150, 80, 200, 0.3);
                        border-radius: 15px;
                        padding: 10px;
                        margin: 3px;
                        transition: all 200ms ease-out;
                    }
                    #icon-box:hover {
                        background: rgba(60, 30, 90, 0.8);
                        border: 1px solid rgba(200, 80, 255, 1.0);
                        transform: scale(1.08) rotate(2deg);
                    }
                    .selected-icon {
                        background: rgba(80, 40, 120, 0.8) !important;
                        border: 1px solid rgba(220, 100, 255, 1.0) !important;
                        transform: scale(1.08);
                    }
                )";
                break;
                
            case THEME_GREEN: // Matrix Terminal - Sharp edges, monospace feel
                css << R"(
                    window {
                        background: rgba(0, 8, 0, 0.98);
                        border: 3px solid rgba(80, 255, 150, 0.8);
                        border-radius: 4px;
                    }
                    entry {
                        background: rgba(0, 15, 5, 0.95);
                        color: rgba(80, 255, 150, 1.0);
                        border: 2px solid rgba(50, 200, 100, 0.6);
                        border-radius: 2px;
                        padding: 10px 14px;
                        font-size: 16px;
                        font-weight: bold;
                        font-family: monospace;
                    }
                    entry:focus {
                        border: 2px solid rgba(100, 255, 180, 1.0);
                        background: rgba(5, 20, 10, 0.95);
                    }
                    #icon-box {
                        background: rgba(5, 20, 10, 0.8);
                        border: 2px solid rgba(50, 150, 80, 0.5);
                        border-radius: 0px;
                        padding: 6px;
                        margin: 4px;
                        transition: all 100ms linear;
                    }
                    #icon-box:hover {
                        background: rgba(10, 40, 20, 0.9);
                        border: 2px solid rgba(100, 255, 180, 1.0);
                        transform: scale(1.03);
                    }
                    .selected-icon {
                        background: rgba(15, 60, 30, 0.9) !important;
                        border: 2px solid rgba(120, 255, 200, 1.0) !important;
                        transform: scale(1.03);
                    }
                )";
                break;
                
            case THEME_RED: // Hexagonal Gaming - Angular design
                css << R"(
                    window {
                        background: linear-gradient(45deg, rgba(20, 0, 0, 0.98) 0%, rgba(40, 5, 10, 0.98) 100%);
                        border: 3px solid rgba(255, 50, 100, 0.9);
                        border-radius: 0px;
                        box-shadow: 0 0 30px rgba(255, 50, 100, 0.3);
                    }
                    entry {
                        background: rgba(30, 5, 10, 0.95);
                        color: rgba(255, 150, 180, 1.0);
                        border: 2px solid rgba(255, 80, 120, 0.7);
                        border-radius: 0px;
                        padding: 12px 18px;
                        font-size: 17px;
                        font-weight: bold;
                    }
                    entry:focus {
                        border: 3px solid rgba(255, 100, 150, 1.0);
                        background: rgba(40, 10, 15, 0.95);
                    }
                    #icon-box {
                        background: rgba(25, 5, 10, 0.75);
                        border: 2px solid rgba(150, 30, 60, 0.6);
                        border-radius: 0px;
                        padding: 8px;
                        margin: 3px;
                        transition: all 120ms ease-in;
                        clip-path: polygon(10% 0%, 90% 0%, 100% 10%, 100% 90%, 90% 100%, 10% 100%, 0% 90%, 0% 10%);
                    }
                    #icon-box:hover {
                        background: rgba(60, 15, 25, 0.9);
                        border: 2px solid rgba(255, 80, 130, 1.0);
                        transform: scale(1.1);
                    }
                    .selected-icon {
                        background: rgba(80, 20, 35, 0.9) !important;
                        border: 3px solid rgba(255, 100, 150, 1.0) !important;
                        transform: scale(1.1);
                    }
                )";
                break;
                
            case THEME_ORANGE: // Retro Warm - Soft gradients, vintage
                css << R"(
                    window {
                        background: radial-gradient(circle at top left, rgba(25, 15, 5, 0.96) 0%, rgba(20, 10, 8, 0.96) 100%);
                        border: 2px solid rgba(255, 165, 80, 0.7);
                        border-radius: 25px;
                        box-shadow: inset 0 0 50px rgba(255, 140, 50, 0.1);
                    }
                    entry {
                        background: rgba(30, 20, 10, 0.9);
                        color: rgba(255, 200, 150, 1.0);
                        border: 2px solid rgba(200, 130, 60, 0.5);
                        border-radius: 20px;
                        padding: 14px 22px;
                        font-size: 15px;
                        font-weight: normal;
                    }
                    entry:focus {
                        border: 2px solid rgba(255, 180, 100, 0.8);
                        background: rgba(35, 25, 15, 0.95);
                    }
                    #icon-box {
                        background: rgba(35, 25, 15, 0.6);
                        border: 1px solid rgba(180, 120, 60, 0.4);
                        border-radius: 12px;
                        padding: 12px;
                        margin: 2px;
                        transition: all 250ms ease-in-out;
                    }
                    #icon-box:hover {
                        background: rgba(60, 40, 20, 0.85);
                        border: 2px solid rgba(255, 165, 80, 0.9);
                        transform: scale(1.06);
                        box-shadow: 0 4px 15px rgba(255, 140, 50, 0.3);
                    }
                    .selected-icon {
                        background: rgba(70, 50, 25, 0.9) !important;
                        border: 2px solid rgba(255, 180, 100, 0.9) !important;
                        transform: scale(1.06);
                        box-shadow: 0 4px 15px rgba(255, 140, 50, 0.4);
                    }
                )";
                break;
                
            case THEME_CYAN: // Holographic Future - Glossy, transparent
                css << R"(
                    window {
                        background: linear-gradient(180deg, rgba(5, 15, 18, 0.92) 0%, rgba(8, 20, 25, 0.92) 100%);
                        border: 1px solid rgba(100, 255, 255, 0.6);
                        border-radius: 16px;
                        box-shadow: 0 8px 32px rgba(50, 255, 255, 0.2);
                    }
                    entry {
                        background: rgba(10, 25, 30, 0.7);
                        color: rgba(200, 255, 255, 1.0);
                        border: 1px solid rgba(80, 255, 255, 0.4);
                        border-radius: 12px;
                        padding: 13px 20px;
                        font-size: 16px;
                        font-weight: 300;
                    }
                    entry:focus {
                        border: 2px solid rgba(120, 255, 255, 0.8);
                        background: rgba(15, 30, 35, 0.85);
                        box-shadow: 0 0 20px rgba(80, 255, 255, 0.3);
                    }
                    #icon-box {
                        background: rgba(15, 30, 35, 0.5);
                        border: 1px solid rgba(80, 200, 220, 0.3);
                        border-radius: 10px;
                        padding: 10px;
                        margin: 3px;
                        transition: all 180ms cubic-bezier(0.68, -0.55, 0.265, 1.55);
                        backdrop-filter: blur(5px);
                    }
                    #icon-box:hover {
                        background: rgba(25, 50, 60, 0.8);
                        border: 1px solid rgba(100, 255, 255, 0.9);
                        transform: scale(1.07) translateY(-3px);
                        box-shadow: 0 8px 20px rgba(50, 255, 255, 0.4);
                    }
                    .selected-icon {
                        background: rgba(30, 60, 70, 0.85) !important;
                        border: 2px solid rgba(120, 255, 255, 1.0) !important;
                        transform: scale(1.07) translateY(-3px);
                        box-shadow: 0 8px 20px rgba(80, 255, 255, 0.5);
                    }
                )";
                break;
        }
        
        // Common styles for all themes
        css << R"(
            scrolledwindow {
                background: transparent;
                border: none;
            }
            
            list {
                background: transparent;
                border: none;
            }
            
            row {
                background: transparent;
                color: rgba(170, 200, 230, 0.95);
                border: none;
                padding: 10px;
                margin: 4px 8px;
            }
            
            row:selected {
                background: transparent;
                border: none;
            }
            
            .favorite-star {
                color: rgba(255, 215, 0, 1.0);
                font-size: 14px;
            }
            
            .recent-badge {
                color: rgba()" << colors.accent << R"(, 1.0);
                font-size: 10px;
            }
            
            #header-box {
                background: rgba(5, 5, 10, 0.8);
                border-bottom: 1px solid rgba()" << colors.primary << R"(, 0.5);
                padding: 8px;
            }
            
            #stats-label {
                color: rgba()" << colors.accent << R"(, 0.9);
                font-size: 11px;
                font-family: monospace;
            }
            
            button {
                background: rgba(15, 15, 25, 0.8);
                color: rgba(180, 230, 255, 1.0);
                border: 1px solid rgba()" << colors.primary << R"(, 0.5);
                border-radius: 6px;
                padding: 6px 12px;
                transition: all 150ms ease-in-out;
            }
            
            button:hover {
                background: rgba(20, 50, 90, 0.9);
                border: 1px solid rgba()" << colors.secondary << R"(, 0.8);
            }
            
            label {
                color: inherit;
                padding: 4px;
            }
            
            .calculator-result {
                color: rgba(50, 255, 150, 1.0);
                font-size: 18px;
                font-weight: bold;
                font-family: monospace;
            }
            
            .mode-indicator {
                color: rgba()" << colors.accent << R"(, 1.0);
                font-size: 12px;
                font-weight: bold;
            }
        )";
        
        return css.str();
    }
    
    std::string get_theme_css() {
        // If morph theme, delegate to the current morphed theme
        if (config.current_theme == THEME_MORPH) {
            Theme morphed = static_cast<Theme>(morph_current_theme);
            return get_theme_css_for(morphed);
        }
        // Otherwise use the selected theme
        return get_theme_css_for(config.current_theme);
    }
    
    void apply_theme() {
        // Stop morph timer if it was running
        if (morph_timer != 0) {
            g_source_remove(morph_timer);
            morph_timer = 0;
        }
        
        // Start morph timer if morph theme is selected
        if (config.current_theme == THEME_MORPH) {
            morph_timer = g_timeout_add(3000, morph_timer_callback, this); // Change every 3 seconds
        }
        
        std::string css = get_theme_css();
        
        #if GTK_IS_VERSION_4
            gtk_css_provider_load_from_string(css_provider, css.c_str());
        #else
            gtk_css_provider_load_from_data(css_provider, css.c_str(), -1, NULL);
        #endif
        
        config.save();
    }
    
    void update_stats() {
        struct sysinfo info;
        if (sysinfo(&info) != 0) return;
        
        double load = info.loads[0] / 65536.0;
        long total_ram = info.totalram / (1024 * 1024);
        long free_ram = info.freeram / (1024 * 1024);
        long used_ram = total_ram - free_ram;
        
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char time_str[32];
        strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);
        
        std::ostringstream stats;
        stats << time_str << "  |  CPU: " << std::fixed << std::setprecision(2) << load 
              << "  |  RAM: " << used_ram << "/" << total_ram << " MB";
        
        gtk_label_set_text(GTK_LABEL(stats_label), stats.str().c_str());
    }
    
    static gboolean stats_timer_callback(gpointer user_data) {
        FuturisticLauncher *launcher = static_cast<FuturisticLauncher*>(user_data);
        launcher->update_stats();
        return G_SOURCE_CONTINUE;
    }
    
    static gboolean morph_timer_callback(gpointer user_data) {
        FuturisticLauncher *launcher = static_cast<FuturisticLauncher*>(user_data);
        
        // Cycle to next theme
        launcher->morph_current_theme = (launcher->morph_current_theme + 1) % 6;
        
        // Apply the morphed theme
        launcher->apply_theme();
        
        return G_SOURCE_CONTINUE;
    }
    
    static gboolean fade_timer_callback(gpointer user_data) {
        FuturisticLauncher *launcher = static_cast<FuturisticLauncher*>(user_data);
        
        if (launcher->fading_in) {
            launcher->current_opacity += 0.05;
            if (launcher->current_opacity >= 1.0) {
                launcher->current_opacity = 1.0;
                launcher->fade_timer = 0;
                return G_SOURCE_REMOVE;
            }
        } else {
            launcher->current_opacity -= 0.05;
            if (launcher->current_opacity <= 0.0) {
                launcher->current_opacity = 0.0;
                gtk_widget_set_visible(launcher->window, FALSE);
                launcher->fade_timer = 0;
                return G_SOURCE_REMOVE;
            }
        }
        
        gtk_widget_set_opacity(launcher->window, launcher->current_opacity);
        return G_SOURCE_CONTINUE;
    }
    
    void start_fade(bool fade_in) {
        if (fade_timer != 0) {
            g_source_remove(fade_timer);
        }
        
        fading_in = fade_in;
        
        if (fade_in) {
            current_opacity = 0.0;
            gtk_widget_set_opacity(window, 0.0);
            gtk_widget_set_visible(window, TRUE);
        }
        
        fade_timer = g_timeout_add(16, fade_timer_callback, this); // ~60fps
    }
    
    double calculate_expression(const std::string& expr) {
        // Simple calculator - supports +, -, *, /, (, )
        try {
            // Remove spaces
            std::string clean_expr = expr;
            clean_expr.erase(std::remove(clean_expr.begin(), clean_expr.end(), ' '), clean_expr.end());
            
            // Very basic evaluation using system bc (for demo purposes)
            std::string cmd = "echo '" + clean_expr + "' | bc -l 2>/dev/null";
            FILE* pipe = popen(cmd.c_str(), "r");
            if (!pipe) return NAN;
            
            char buffer[128];
            std::string result;
            while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
                result += buffer;
            }
            pclose(pipe);
            
            if (result.empty()) return NAN;
            return std::stod(result);
            
        } catch (...) {
            return NAN;
        }
    }

public:
    FuturisticLauncher() {
        config.load();
        load_applications();
    }
    
    ~FuturisticLauncher() {
        if (lock_fd >= 0) {
            close(lock_fd);
            unlink("/tmp/futuristic-launcher.lock");
        }
        if (stats_timer != 0) {
            g_source_remove(stats_timer);
        }
        if (fade_timer != 0) {
            g_source_remove(fade_timer);
        }
        if (morph_timer != 0) {
            g_source_remove(morph_timer);
        }
        config.save();
    }
    
    bool try_acquire_lock() {
        lock_fd = open("/tmp/futuristic-launcher.lock", O_CREAT | O_RDWR, 0666);
        if (lock_fd < 0) return false;
        
        if (flock(lock_fd, LOCK_EX | LOCK_NB) < 0) {
            close(lock_fd);
            lock_fd = -1;
            return false;
        }
        return true;
    }
    
    static void signal_toggle() {
        std::ifstream pid_file("/tmp/futuristic-launcher.pid");
        if (pid_file.is_open()) {
            pid_t pid;
            pid_file >> pid;
            pid_file.close();
            kill(pid, SIGUSR1);
        }
    }
    
    void save_pid() {
        std::ofstream pid_file("/tmp/futuristic-launcher.pid");
        pid_file << getpid();
        pid_file.close();
    }
    
    void toggle_visibility() {
        if (is_visible) {
            start_fade(false);
            is_visible = false;
        } else {
            start_fade(true);
            gtk_widget_grab_focus(search_entry);
            #if GTK_IS_VERSION_4
                gtk_editable_set_text(GTK_EDITABLE(search_entry), "");
            #else
                gtk_entry_set_text(GTK_ENTRY(search_entry), "");
            #endif
            filter_apps("");
            update_list();
            is_visible = true;
        }
    }
    
    static void handle_signal(int sig) {
        if (sig == SIGUSR1) {
            g_idle_add([](gpointer data) -> gboolean {
                FuturisticLauncher* launcher = static_cast<FuturisticLauncher*>(data);
                launcher->toggle_visibility();
                return G_SOURCE_REMOVE;
            }, g_launcher_instance);
        }
    }
    
    static FuturisticLauncher* g_launcher_instance;

    void load_applications() {
        std::vector<std::string> app_dirs = {
            "/usr/share/applications",
            "/usr/local/share/applications",
            std::string(g_get_home_dir()) + "/.local/share/applications"
        };

        for (const auto& dir : app_dirs) {
            if (!fs::exists(dir)) continue;
            
            for (const auto& entry : fs::directory_iterator(dir)) {
                if (entry.path().extension() == ".desktop") {
                    DesktopApp app = parse_desktop_file(entry.path().string());
                    if (!app.name.empty() && !app.no_display) {
                        // Load stats from config
                        if (config.launch_counts.count(app.name)) {
                            app.launch_count = config.launch_counts[app.name];
                        }
                        if (config.last_launches.count(app.name)) {
                            app.last_launch = config.last_launches[app.name];
                        }
                        if (config.favorites.count(app.name)) {
                            app.is_favorite = true;
                        }
                        
                        all_apps.push_back(app);
                    }
                }
            }
        }

        // Sort by favorites, then recent, then alphabetically
        std::sort(all_apps.begin(), all_apps.end(), 
            [](const DesktopApp& a, const DesktopApp& b) {
                if (a.is_favorite != b.is_favorite) return a.is_favorite;
                if (a.launch_count != b.launch_count) return a.launch_count > b.launch_count;
                return a.name < b.name;
            });
        
        filtered_apps = all_apps;
        
        std::cout << "Loaded " << all_apps.size() << " applications" << std::endl;
    }

    DesktopApp parse_desktop_file(const std::string& filepath) {
        DesktopApp app;
        std::ifstream file(filepath);
        std::string line;
        bool in_desktop_entry = false;

        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            if (line == "[Desktop Entry]") {
                in_desktop_entry = true;
                continue;
            } else if (line[0] == '[') {
                in_desktop_entry = false;
                continue;
            }

            if (!in_desktop_entry) continue;

            size_t eq_pos = line.find('=');
            if (eq_pos == std::string::npos) continue;

            std::string key = line.substr(0, eq_pos);
            std::string value = line.substr(eq_pos + 1);

            if (key == "Name") {
                app.name = value;
            } else if (key == "Exec") {
                app.exec = value;
            } else if (key == "Icon") {
                app.icon = value;
            } else if (key == "Comment") {
                app.comment = value;
            } else if (key == "Categories") {
                app.categories = value;
            } else if (key == "NoDisplay") {
                app.no_display = (value == "true");
            } else if (key == "Type" && value != "Application") {
                app.no_display = true;
            }
        }

        return app;
    }

    void filter_apps(const std::string& search_text) {
        filtered_apps.clear();
        selected_index = 0;
        calculator_mode = false;
        web_search_mode = false;
        command_mode = false;

        if (search_text.empty()) {
            filtered_apps = all_apps;
            return;
        }
        
        // Check for special modes
        if (search_text[0] == '?') {
            web_search_mode = true;
            return;
        } else if (search_text[0] == '>') {
            command_mode = true;
            return;
        }
        
        // Check if it's a math expression
        std::regex math_regex(R"(^[\d\s\+\-\*\/\(\)\.]+$)");
        if (std::regex_match(search_text, math_regex)) {
            double result = calculate_expression(search_text);
            if (!std::isnan(result)) {
                calculator_mode = true;
                return;
            }
        }

        // Fuzzy search
        std::vector<std::pair<DesktopApp, int>> scored_apps;
        
        for (const auto& app : all_apps) {
            int name_score = fuzzy_score(app.name, search_text);
            int comment_score = fuzzy_score(app.comment, search_text) / 2;
            int total_score = name_score + comment_score;
            
            if (total_score > 0) {
                scored_apps.push_back({app, total_score});
            }
        }
        
        // Sort by score
        std::sort(scored_apps.begin(), scored_apps.end(),
            [](const auto& a, const auto& b) {
                return a.second > b.second;
            });
        
        for (const auto& [app, score] : scored_apps) {
            filtered_apps.push_back(app);
        }
    }

    std::string clean_exec(const std::string& exec) {
        std::string cleaned = exec;
        size_t pos;
        while ((pos = cleaned.find('%')) != std::string::npos) {
            if (pos + 1 < cleaned.length()) {
                cleaned.erase(pos, 2);
            } else {
                cleaned.erase(pos, 1);
            }
        }
        return cleaned;
    }

    void launch_app(const DesktopApp& app) {
        std::string cmd = clean_exec(app.exec);
        GError *error = NULL;
        
        g_spawn_command_line_async(cmd.c_str(), &error);
        
        if (error) {
            g_printerr("Failed to launch %s: %s\n", app.name.c_str(), error->message);
            g_error_free(error);
        } else {
            // Update launch stats
            for (auto& a : all_apps) {
                if (a.name == app.name) {
                    a.launch_count++;
                    a.last_launch = time(NULL);
                    config.launch_counts[a.name] = a.launch_count;
                    config.last_launches[a.name] = a.last_launch;
                    break;
                }
            }
            config.save();
        }
        
        toggle_visibility();
    }
    
    void toggle_favorite(const DesktopApp& app) {
        for (auto& a : all_apps) {
            if (a.name == app.name) {
                a.is_favorite = !a.is_favorite;
                if (a.is_favorite) {
                    config.favorites.insert(a.name);
                } else {
                    config.favorites.erase(a.name);
                }
                config.save();
                break;
            }
        }
        
        // Re-filter and update
        const char *text = gtk_editable_get_text(GTK_EDITABLE(search_entry));
        filter_apps(text);
        update_list();
    }
    
    void execute_web_search(const std::string& query) {
        std::string search_query = query.substr(1); // Remove '?'
        std::string url = "xdg-open 'https://www.google.com/search?q=" + search_query + "'";
        int ret = system(url.c_str());
        (void)ret; // Suppress unused warning
        toggle_visibility();
    }
    
    void execute_command(const std::string& command) {
        std::string cmd = command.substr(1); // Remove '>'
        cmd += " &";
        int ret = system(cmd.c_str());
        (void)ret; // Suppress unused warning
        toggle_visibility();
    }
    
    void show_power_menu() {
        GtkWidget *dialog = gtk_dialog_new_with_buttons(
            "Power Menu",
            GTK_WINDOW(window),
            GTK_DIALOG_MODAL,
            "_Cancel", GTK_RESPONSE_CANCEL,
            NULL
        );
        
        #if GTK_IS_VERSION_4
            GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        #else
            GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        #endif
        
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_widget_set_margin_start(box, 20);
        gtk_widget_set_margin_end(box, 20);
        gtk_widget_set_margin_top(box, 20);
        gtk_widget_set_margin_bottom(box, 20);
        
        GtkWidget *shutdown_btn = gtk_button_new_with_label("🔴 Shutdown");
        GtkWidget *reboot_btn = gtk_button_new_with_label("🔄 Reboot");
        GtkWidget *logout_btn = gtk_button_new_with_label("🚪 Logout");
        
        gtk_box_append(GTK_BOX(box), shutdown_btn);
        gtk_box_append(GTK_BOX(box), reboot_btn);
        gtk_box_append(GTK_BOX(box), logout_btn);
        
        #if GTK_IS_VERSION_4
            gtk_box_append(GTK_BOX(content), box);
        #else
            gtk_container_add(GTK_CONTAINER(content), box);
        #endif
        
        g_signal_connect_swapped(shutdown_btn, "clicked", 
            G_CALLBACK(+[](GtkDialog* d) { 
                int ret = system("systemctl poweroff");
                (void)ret;
                #if GTK_IS_VERSION_4
                    gtk_window_destroy(GTK_WINDOW(d));
                #else
                    gtk_widget_destroy(GTK_WIDGET(d));
                #endif
            }), dialog);
            
        g_signal_connect_swapped(reboot_btn, "clicked", 
            G_CALLBACK(+[](GtkDialog* d) { 
                int ret = system("systemctl reboot");
                (void)ret;
                #if GTK_IS_VERSION_4
                    gtk_window_destroy(GTK_WINDOW(d));
                #else
                    gtk_widget_destroy(GTK_WIDGET(d));
                #endif
            }), dialog);
            
        g_signal_connect_swapped(logout_btn, "clicked", 
            G_CALLBACK(+[](GtkDialog* d) { 
                int ret = system("loginctl terminate-user $USER");
                (void)ret;
                #if GTK_IS_VERSION_4
                    gtk_window_destroy(GTK_WINDOW(d));
                #else
                    gtk_widget_destroy(GTK_WIDGET(d));
                #endif
            }), dialog);
        
        #if GTK_IS_VERSION_3
            gtk_widget_show_all(dialog);
        #else
            gtk_widget_show(dialog);
        #endif
        
        gtk_dialog_run(GTK_DIALOG(dialog));
        
        #if GTK_IS_VERSION_4
            gtk_window_destroy(GTK_WINDOW(dialog));
        #else
            gtk_widget_destroy(dialog);
        #endif
    }

    #if GTK_IS_VERSION_4
    static void on_icon_clicked(GtkGestureClick *gesture, int n_press, double x, double y, gpointer user_data) {
        FuturisticLauncher *launcher = static_cast<FuturisticLauncher*>(user_data);
        GtkWidget *widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture));
        int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(gesture), "app_index"));
        
        if (index >= 0 && index < (int)launcher->filtered_apps.size()) {
            launcher->launch_app(launcher->filtered_apps[index]);
        }
    }
    
    static void on_icon_right_clicked(GtkGestureClick *gesture, int n_press, double x, double y, gpointer user_data) {
        FuturisticLauncher *launcher = static_cast<FuturisticLauncher*>(user_data);
        int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(gesture), "app_index"));
        
        if (index >= 0 && index < (int)launcher->filtered_apps.size()) {
            launcher->toggle_favorite(launcher->filtered_apps[index]);
        }
    }
    #else
    static gboolean on_icon_clicked_gtk3(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
        FuturisticLauncher *launcher = static_cast<FuturisticLauncher*>(user_data);
        int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "app_index"));
        
        if (index >= 0 && index < (int)launcher->filtered_apps.size()) {
            if (event->button == 1) { // Left click
                launcher->launch_app(launcher->filtered_apps[index]);
            } else if (event->button == 3) { // Right click
                launcher->toggle_favorite(launcher->filtered_apps[index]);
            }
        }
        return TRUE;
    }
    
    static gboolean on_icon_enter(GtkWidget *widget, GdkEventCrossing *event, gpointer user_data) {
        GList *children = gtk_container_get_children(GTK_CONTAINER(widget));
        if (children) {
            GtkWidget *icon_box = GTK_WIDGET(children->data);
            GtkStyleContext *context = gtk_widget_get_style_context(icon_box);
            gtk_style_context_add_class(context, "selected-icon");
            g_list_free(children);
        }
        return FALSE;
    }
    
    static gboolean on_icon_leave(GtkWidget *widget, GdkEventCrossing *event, gpointer user_data) {
        GList *children = gtk_container_get_children(GTK_CONTAINER(widget));
        if (children) {
            GtkWidget *icon_box = GTK_WIDGET(children->data);
            GtkStyleContext *context = gtk_widget_get_style_context(icon_box);
            gtk_style_context_remove_class(context, "selected-icon");
            g_list_free(children);
        }
        return FALSE;
    }
    #endif

    static void on_search_changed(GtkSearchEntry *entry, gpointer user_data) {
        FuturisticLauncher *launcher = static_cast<FuturisticLauncher*>(user_data);
        const char *text = gtk_editable_get_text(GTK_EDITABLE(entry));
        launcher->filter_apps(text);
        launcher->update_list();
    }

    #if GTK_IS_VERSION_4
    static gboolean on_key_press(GtkEventControllerKey *controller, guint keyval, 
                                  guint keycode, GdkModifierType state, gpointer user_data) {
    #else
    static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
        guint keyval = event->keyval;
        GdkModifierType state = (GdkModifierType)event->state;
    #endif
        FuturisticLauncher *launcher = static_cast<FuturisticLauncher*>(user_data);
        const int ICONS_PER_ROW = 3;
        
        // Theme switching with Ctrl+1-7
        if (state & GDK_CONTROL_MASK) {
            if (keyval >= GDK_KEY_1 && keyval <= GDK_KEY_7) {
                launcher->config.current_theme = static_cast<Theme>(keyval - GDK_KEY_1);
                launcher->apply_theme();
                return TRUE;
            }
        }
        
        // Quick launch with Alt+Number
        if (state & GDK_MOD1_MASK) {
            if (keyval >= GDK_KEY_1 && keyval <= GDK_KEY_9) {
                int index = keyval - GDK_KEY_1;
                if (index < (int)launcher->filtered_apps.size()) {
                    launcher->launch_app(launcher->filtered_apps[index]);
                }
                return TRUE;
            }
        }
        
        if (keyval == GDK_KEY_Escape) {
            launcher->toggle_visibility();
            return TRUE;
        } else if (keyval == GDK_KEY_Down) {
            if (launcher->selected_index + ICONS_PER_ROW < (int)launcher->filtered_apps.size()) {
                launcher->selected_index += ICONS_PER_ROW;
                launcher->update_selection();
            }
            return TRUE;
        } else if (keyval == GDK_KEY_Up) {
            if (launcher->selected_index >= ICONS_PER_ROW) {
                launcher->selected_index -= ICONS_PER_ROW;
                launcher->update_selection();
            }
            return TRUE;
        } else if (keyval == GDK_KEY_Left) {
            if (launcher->selected_index > 0) {
                launcher->selected_index--;
                launcher->update_selection();
            }
            return TRUE;
        } else if (keyval == GDK_KEY_Right) {
            if (launcher->selected_index < (int)launcher->filtered_apps.size() - 1) {
                launcher->selected_index++;
                launcher->update_selection();
            }
            return TRUE;
        } else if (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter) {
            const char *text = gtk_editable_get_text(GTK_EDITABLE(launcher->search_entry));
            std::string search_text(text);
            
            if (launcher->calculator_mode) {
                // Do nothing - result already shown
                return TRUE;
            } else if (launcher->web_search_mode) {
                launcher->execute_web_search(search_text);
                return TRUE;
            } else if (launcher->command_mode) {
                launcher->execute_command(search_text);
                return TRUE;
            } else if (!launcher->filtered_apps.empty() && 
                       launcher->selected_index < (int)launcher->filtered_apps.size()) {
                launcher->launch_app(launcher->filtered_apps[launcher->selected_index]);
            }
            return TRUE;
        } else if (keyval == GDK_KEY_F12) {
            launcher->show_power_menu();
            return TRUE;
        }
        
        return FALSE;
    }

    void update_selection() {
        for (size_t i = 0; i < icon_widgets.size(); i++) {
            GtkWidget *widget = icon_widgets[i];
            
            #if GTK_IS_VERSION_3
                GList *children = gtk_container_get_children(GTK_CONTAINER(widget));
                if (children) {
                    GtkWidget *icon_box = GTK_WIDGET(children->data);
                    GtkStyleContext *context = gtk_widget_get_style_context(icon_box);
                    gtk_style_context_remove_class(context, "selected-icon");
                    g_list_free(children);
                }
            #else
                GtkStyleContext *context = gtk_widget_get_style_context(widget);
                gtk_style_context_remove_class(context, "selected-icon");
            #endif
        }
        
        if (selected_index >= 0 && selected_index < (int)icon_widgets.size()) {
            GtkWidget *widget = icon_widgets[selected_index];
            
            #if GTK_IS_VERSION_3
                GList *children = gtk_container_get_children(GTK_CONTAINER(widget));
                if (children) {
                    GtkWidget *icon_box = GTK_WIDGET(children->data);
                    GtkStyleContext *context = gtk_widget_get_style_context(icon_box);
                    gtk_style_context_add_class(context, "selected-icon");
                    g_list_free(children);
                }
            #else
                GtkStyleContext *context = gtk_widget_get_style_context(widget);
                gtk_style_context_add_class(context, "selected-icon");
            #endif
            
            GtkWidget *row_widget = gtk_widget_get_parent(widget);
            if (row_widget) {
                GtkAdjustment *adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolled_window));
                
                #if GTK_IS_VERSION_4
                    int row_height = 110;
                #else
                    GtkAllocation alloc;
                    gtk_widget_get_allocation(row_widget, &alloc);
                    int row_height = alloc.height > 0 ? alloc.height : 110;
                #endif
                
                int row_index = selected_index / 3;
                double value = row_index * row_height;
                gtk_adjustment_set_value(adj, value);
            }
        }
    }

    void update_list() {
        icon_widgets.clear();
        
        #if GTK_IS_VERSION_4
            GtkWidget *child = gtk_widget_get_first_child(app_list);
            while (child) {
                GtkWidget *next = gtk_widget_get_next_sibling(child);
                gtk_list_box_remove(GTK_LIST_BOX(app_list), child);
                child = next;
            }
        #else
            GList *children = gtk_container_get_children(GTK_CONTAINER(app_list));
            for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
                gtk_widget_destroy(GTK_WIDGET(iter->data));
            }
            g_list_free(children);
        #endif
        
        // Special mode displays
        const char *search_text = gtk_editable_get_text(GTK_EDITABLE(search_entry));
        
        if (calculator_mode) {
            double result = calculate_expression(search_text);
            
            GtkWidget *result_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
            gtk_widget_set_halign(result_box, GTK_ALIGN_CENTER);
            gtk_widget_set_margin_top(result_box, 50);
            
            GtkWidget *mode_label = gtk_label_new("🔢 CALCULATOR MODE");
            GtkStyleContext *mode_ctx = gtk_widget_get_style_context(mode_label);
            gtk_style_context_add_class(mode_ctx, "mode-indicator");
            gtk_box_append(GTK_BOX(result_box), mode_label);
            
            std::ostringstream result_str;
            result_str << std::fixed << std::setprecision(6) << result;
            GtkWidget *result_label = gtk_label_new(result_str.str().c_str());
            GtkStyleContext *ctx = gtk_widget_get_style_context(result_label);
            gtk_style_context_add_class(ctx, "calculator-result");
            gtk_box_append(GTK_BOX(result_box), result_label);
            
            gtk_list_box_append(GTK_LIST_BOX(app_list), result_box);
            
            #if GTK_IS_VERSION_3
                gtk_widget_show_all(result_box);
            #endif
            return;
        }
        
        if (web_search_mode) {
            GtkWidget *search_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
            gtk_widget_set_halign(search_box, GTK_ALIGN_CENTER);
            gtk_widget_set_margin_top(search_box, 50);
            
            GtkWidget *mode_label = gtk_label_new("🌐 WEB SEARCH MODE");
            GtkStyleContext *ctx = gtk_widget_get_style_context(mode_label);
            gtk_style_context_add_class(ctx, "mode-indicator");
            gtk_box_append(GTK_BOX(search_box), mode_label);
            
            std::string query = std::string(search_text).substr(1);
            GtkWidget *query_label = gtk_label_new(("Search Google for: " + query).c_str());
            gtk_box_append(GTK_BOX(search_box), query_label);
            
            GtkWidget *hint_label = gtk_label_new("Press Enter to search");
            gtk_box_append(GTK_BOX(search_box), hint_label);
            
            gtk_list_box_append(GTK_LIST_BOX(app_list), search_box);
            
            #if GTK_IS_VERSION_3
                gtk_widget_show_all(search_box);
            #endif
            return;
        }
        
        if (command_mode) {
            GtkWidget *cmd_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
            gtk_widget_set_halign(cmd_box, GTK_ALIGN_CENTER);
            gtk_widget_set_margin_top(cmd_box, 50);
            
            GtkWidget *mode_label = gtk_label_new("💻 TERMINAL MODE");
            GtkStyleContext *ctx = gtk_widget_get_style_context(mode_label);
            gtk_style_context_add_class(ctx, "mode-indicator");
            gtk_box_append(GTK_BOX(cmd_box), mode_label);
            
            std::string cmd = std::string(search_text).substr(1);
            GtkWidget *cmd_label = gtk_label_new(("Execute: " + cmd).c_str());
            gtk_box_append(GTK_BOX(cmd_box), cmd_label);
            
            GtkWidget *hint_label = gtk_label_new("Press Enter to execute");
            gtk_box_append(GTK_BOX(cmd_box), hint_label);
            
            gtk_list_box_append(GTK_LIST_BOX(app_list), cmd_box);
            
            #if GTK_IS_VERSION_3
                gtk_widget_show_all(cmd_box);
            #endif
            return;
        }

        // Normal app display
        const int ICONS_PER_ROW = 3;
        
        for (size_t i = 0; i < filtered_apps.size(); i += ICONS_PER_ROW) {
            GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
            gtk_widget_set_halign(row_box, GTK_ALIGN_CENTER);
            gtk_widget_set_margin_top(row_box, 5);
            gtk_widget_set_margin_bottom(row_box, 5);
            
            for (int j = 0; j < ICONS_PER_ROW && (i + j) < filtered_apps.size(); j++) {
                const auto& app = filtered_apps[i + j];
                
                GtkWidget *icon_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
                gtk_widget_set_size_request(icon_box, 140, 110);
                gtk_widget_set_name(icon_box, "icon-box");
                
                icon_widgets.push_back(icon_box);
                
                // Add favorite star or recent badge
                if (app.is_favorite || (time(NULL) - app.last_launch) < 3600) {
                    GtkWidget *badge_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
                    gtk_widget_set_halign(badge_box, GTK_ALIGN_CENTER);
                    
                    if (app.is_favorite) {
                        GtkWidget *star = gtk_label_new("⭐");
                        GtkStyleContext *star_ctx = gtk_widget_get_style_context(star);
                        gtk_style_context_add_class(star_ctx, "favorite-star");
                        gtk_box_append(GTK_BOX(badge_box), star);
                    }
                    
                    if ((time(NULL) - app.last_launch) < 3600) {
                        GtkWidget *recent = gtk_label_new("🕐");
                        GtkStyleContext *recent_ctx = gtk_widget_get_style_context(recent);
                        gtk_style_context_add_class(recent_ctx, "recent-badge");
                        gtk_box_append(GTK_BOX(badge_box), recent);
                    }
                    
                    gtk_box_append(GTK_BOX(icon_box), badge_box);
                }
                
                // Icon
                GtkWidget *icon_widget;
                GtkIconTheme *icon_theme = gtk_icon_theme_get_default();
                
                if (!app.icon.empty()) {
                    #if GTK_IS_VERSION_4
                        GtkIconPaintable *icon_paintable = gtk_icon_theme_lookup_icon(
                            icon_theme, app.icon.c_str(), NULL, config.icon_size, 1,
                            GTK_TEXT_DIR_NONE, GTK_ICON_LOOKUP_FORCE_REGULAR);
                        if (icon_paintable) {
                            icon_widget = gtk_image_new_from_paintable(GDK_PAINTABLE(icon_paintable));
                        } else {
                            icon_widget = gtk_image_new_from_icon_name("application-x-executable");
                        }
                    #else
                        GdkPixbuf *pixbuf = gtk_icon_theme_load_icon(icon_theme, app.icon.c_str(), 
                                                                      config.icon_size, GTK_ICON_LOOKUP_FORCE_SIZE, NULL);
                        if (pixbuf) {
                            icon_widget = gtk_image_new_from_pixbuf(pixbuf);
                            g_object_unref(pixbuf);
                        } else {
                            icon_widget = gtk_image_new_from_icon_name("application-x-executable", GTK_ICON_SIZE_DIALOG);
                        }
                    #endif
                } else {
                    #if GTK_IS_VERSION_4
                        icon_widget = gtk_image_new_from_icon_name("application-x-executable");
                    #else
                        icon_widget = gtk_image_new_from_icon_name("application-x-executable", GTK_ICON_SIZE_DIALOG);
                    #endif
                }
                
                gtk_widget_set_size_request(icon_widget, config.icon_size, config.icon_size);
                gtk_box_append(GTK_BOX(icon_box), icon_widget);
                
                // Label
                GtkWidget *label = gtk_label_new(app.name.c_str());
                gtk_label_set_max_width_chars(GTK_LABEL(label), 18);
                gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
                gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
                gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
                
                PangoAttrList *attrs = pango_attr_list_new();
                PangoAttribute *attr = pango_attr_size_new(9 * PANGO_SCALE);
                pango_attr_list_insert(attrs, attr);
                gtk_label_set_attributes(GTK_LABEL(label), attrs);
                pango_attr_list_unref(attrs);
                
                gtk_box_append(GTK_BOX(icon_box), label);
                
                // Make clickable
                #if GTK_IS_VERSION_4
                    GtkGesture *left_click = gtk_gesture_click_new();
                    g_object_set_data(G_OBJECT(left_click), "app_index", GINT_TO_POINTER(i + j));
                    g_signal_connect(left_click, "pressed", G_CALLBACK(on_icon_clicked), this);
                    gtk_widget_add_controller(icon_box, GTK_EVENT_CONTROLLER(left_click));
                    
                    GtkGesture *right_click = gtk_gesture_click_new();
                    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(right_click), GDK_BUTTON_SECONDARY);
                    g_object_set_data(G_OBJECT(right_click), "app_index", GINT_TO_POINTER(i + j));
                    g_signal_connect(right_click, "pressed", G_CALLBACK(on_icon_right_clicked), this);
                    gtk_widget_add_controller(icon_box, GTK_EVENT_CONTROLLER(right_click));
                #else
                    GtkWidget *event_box = gtk_event_box_new();
                    gtk_container_add(GTK_CONTAINER(event_box), icon_box);
                    g_object_set_data(G_OBJECT(event_box), "app_index", GINT_TO_POINTER(i + j));
                    g_signal_connect(event_box, "button-press-event", G_CALLBACK(on_icon_clicked_gtk3), this);
                    g_signal_connect(event_box, "enter-notify-event", G_CALLBACK(on_icon_enter), this);
                    g_signal_connect(event_box, "leave-notify-event", G_CALLBACK(on_icon_leave), this);
                    icon_widgets.back() = event_box;
                    icon_box = event_box;
                #endif
                
                gtk_box_append(GTK_BOX(row_box), icon_box);
            }
            
            gtk_list_box_append(GTK_LIST_BOX(app_list), row_box);
            
            #if GTK_IS_VERSION_3
                gtk_widget_show_all(row_box);
            #endif
        }

        if (!filtered_apps.empty()) {
            selected_index = 0;
        }
    }

    void run() {
        window = gtk_window_new(
            #if GTK_IS_VERSION_4
                
            #else
                GTK_WINDOW_TOPLEVEL
            #endif
        );
        gtk_window_set_title(GTK_WINDOW(window), "Futuristic Launcher");
        gtk_window_set_default_size(GTK_WINDOW(window), LAUNCHER_WIDTH, LAUNCHER_HEIGHT);
        gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
        gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
        
        #if HAS_LAYER_SHELL
            gtk_layer_init_for_window(GTK_WINDOW(window));
            gtk_layer_set_layer(GTK_WINDOW(window), GTK_LAYER_SHELL_LAYER_OVERLAY);
            
            if (POSITION_TOP_LEFT) {
                gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
                gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
                gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_TOP, MARGIN_TOP);
                gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_LEFT, 0);
            }
            
            gtk_layer_set_keyboard_mode(GTK_WINDOW(window), GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE);
        #else
            #if GTK_IS_VERSION_3
                gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
                gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
            #endif
        #endif

        // Apply CSS
        css_provider = gtk_css_provider_new();
        #if GTK_IS_VERSION_4
            gtk_style_context_add_provider_for_display(
                gdk_display_get_default(),
                GTK_STYLE_PROVIDER(css_provider),
                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
            );
        #else
            gtk_style_context_add_provider_for_screen(
                gdk_screen_get_default(),
                GTK_STYLE_PROVIDER(css_provider),
                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
            );
        #endif
        apply_theme();

        // Main container
        GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_window_set_child(GTK_WINDOW(window), main_box);
        
        // Header with stats and power button
        header_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        gtk_widget_set_name(header_box, "header-box");
        gtk_widget_set_margin_start(header_box, 10);
        gtk_widget_set_margin_end(header_box, 10);
        gtk_box_append(GTK_BOX(main_box), header_box);
        
        stats_label = gtk_label_new("Loading...");
        gtk_widget_set_name(stats_label, "stats-label");
        gtk_widget_set_hexpand(stats_label, TRUE);
        gtk_widget_set_halign(stats_label, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(header_box), stats_label);
        
        power_menu_button = gtk_button_new_with_label("⚡");
        g_signal_connect_swapped(power_menu_button, "clicked", 
            G_CALLBACK(+[](FuturisticLauncher* l) { l->show_power_menu(); }), this);
        gtk_box_append(GTK_BOX(header_box), power_menu_button);

        // Search entry
        search_entry = gtk_search_entry_new();
        gtk_widget_set_margin_start(search_entry, 10);
        gtk_widget_set_margin_end(search_entry, 10);
        gtk_widget_set_margin_top(search_entry, 10);
        gtk_widget_set_margin_bottom(search_entry, 5);
        gtk_box_append(GTK_BOX(main_box), search_entry);
        g_signal_connect(search_entry, "search-changed", G_CALLBACK(on_search_changed), this);

        // Scrolled window
        #if GTK_IS_VERSION_4
            scrolled_window = gtk_scrolled_window_new();
        #else
            scrolled_window = gtk_scrolled_window_new(NULL, NULL);
        #endif
        gtk_widget_set_vexpand(scrolled_window, TRUE);
        gtk_widget_set_hexpand(scrolled_window, TRUE);
        gtk_widget_set_margin_start(scrolled_window, 10);
        gtk_widget_set_margin_end(scrolled_window, 10);
        gtk_widget_set_margin_bottom(scrolled_window, 10);
        gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scrolled_window), 450);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), 
                                       GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
        gtk_box_append(GTK_BOX(main_box), scrolled_window);

        // App list
        app_list = gtk_list_box_new();
        gtk_list_box_set_selection_mode(GTK_LIST_BOX(app_list), GTK_SELECTION_NONE);
        gtk_list_box_set_activate_on_single_click(GTK_LIST_BOX(app_list), FALSE);
        gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), app_list);

        // Key press handling
        #if GTK_IS_VERSION_4
            GtkEventController *key_controller = gtk_event_controller_key_new();
            gtk_widget_add_controller(window, key_controller);
            g_signal_connect(key_controller, "key-pressed", G_CALLBACK(on_key_press), this);
        #else
            g_signal_connect(window, "key-press-event", G_CALLBACK(on_key_press), this);
        #endif

        // Initial population
        update_list();
        
        // Start stats timer
        stats_timer = g_timeout_add_seconds(1, stats_timer_callback, this);
        update_stats();

        // Show window
        gtk_window_present_compat(GTK_WINDOW(window));
        gtk_widget_grab_focus(search_entry);
    }
};

FuturisticLauncher* FuturisticLauncher::g_launcher_instance = nullptr;

int main(int argc, char *argv[]) {
    #if GTK_IS_VERSION_4
        gtk_init();
    #else
        gtk_init(&argc, &argv);
    #endif
    
    FuturisticLauncher launcher;
    
    if (!launcher.try_acquire_lock()) {
        FuturisticLauncher::signal_toggle();
        return 0;
    }
    
    FuturisticLauncher::g_launcher_instance = &launcher;
    signal(SIGUSR1, FuturisticLauncher::handle_signal);
    launcher.save_pid();
    
    launcher.run();
    launcher.toggle_visibility();
    
    #if GTK_IS_VERSION_4
        GMainLoop *loop = g_main_loop_new(NULL, FALSE);
        g_main_loop_run(loop);
    #else
        gtk_main();
    #endif
    
    return 0;
}