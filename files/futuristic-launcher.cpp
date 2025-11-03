/*
 * Futuristic Launcher with Shader Background
 * 
 * Build: g++ futuristic-launcher-shader.cpp -o futuristic-launcher `pkg-config --cflags --libs gtk4 gtk4-layer-shell-0 epoxy` -std=c++17
 * Alternative for GTK3: g++ futuristic-launcher-shader.cpp -o futuristic-launcher `pkg-config --cflags --libs gtk+-3.0 gtk-layer-shell-0 epoxy` -std=c++17
 * 
 * Requires: gtk-layer-shell, epoxy for OpenGL
 * 
 * MIT License
 */

#include <gtk/gtk.h>
#include <epoxy/gl.h>
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
    
    static inline void gtk_overlay_set_child(GtkOverlay* overlay, GtkWidget* child) {
        gtk_container_add(GTK_CONTAINER(overlay), child);
    }
    
    static inline void gtk_overlay_add_overlay_compat(GtkOverlay* overlay, GtkWidget* widget) {
        gtk_overlay_add_overlay(overlay, widget);
    }
#else
    static inline void gtk_list_box_remove_compat(GtkListBox* lb, GtkWidget* child) {
        gtk_list_box_remove(lb, child);
    }
    
    static inline void gtk_window_present_compat(GtkWindow* window) {
        gtk_window_present(window);
    }
    
    static inline void gtk_overlay_add_overlay_compat(GtkOverlay* overlay, GtkWidget* widget) {
        gtk_overlay_add_overlay(overlay, widget);
    }
#endif

// Shader source
const char* vertex_shader_source = R"(
    #version 330 core
    layout(location = 0) in vec2 position;
    out vec2 fragCoord;
    void main() {
        fragCoord = position * 0.5 + 0.5;
        gl_Position = vec4(position, 0.0, 1.0);
    }
)";

const char* fragment_shader_source = R"(
    #version 330 core
    precision highp float;
   
    in vec2 fragCoord;
    out vec4 fragColor;
   
    uniform float time;
    uniform vec2 resolution;

    #define iTime time
    #define iResolution resolution

    mat2 rot(in float a){float c = cos(a), s = sin(a);return mat2(c,s,-s,c);}
    const mat3 m3 = mat3(0.33338, 0.56034, -0.71817, -0.87887, 0.32651, -0.15323, 0.15162, 0.69596, 0.61339)*1.93;
    float mag2(vec2 p){return dot(p,p);}
    float linstep(in float mn, in float mx, in float x){ return clamp((x - mn)/(mx - mn), 0., 1.); }
    float prm1 = 0.;
    vec2 bsMo = vec2(0);

    vec2 disp(float t){ return vec2(sin(t*0.22)*1., cos(t*0.175)*1.)*2.; }

    vec2 map(vec3 p)
    {
        vec3 p2 = p;
        p2.xy -= disp(p.z).xy;
        p.xy *= rot(sin(p.z+iTime)*(0.1 + prm1*0.05) + iTime*0.09);
        float cl = mag2(p2.xy);
        float d = 0.;
        p *= .61;
        float z = 1.;
        float trk = 1.;
        float dspAmp = 0.1 + prm1*0.2;
        for(int i = 0; i < 5; i++)
        {
            p += sin(p.zxy*0.75*trk + iTime*trk*.8)*dspAmp;
            d -= abs(dot(cos(p), sin(p.yzx))*z);
            z *= 0.57;
            trk *= 1.4;
            p = p*m3;
        }
        d = abs(d + prm1*3.)+ prm1*.3 - 2.5 + bsMo.y;
        return vec2(d + cl*.2 + 0.25, cl);
    }

    vec4 render( in vec3 ro, in vec3 rd, float time )
    {
        vec4 rez = vec4(0);
        const float ldst = 8.;
        vec3 lpos = vec3(disp(time + ldst)*0.5, time + ldst);
        float t = 1.5;
        float fogT = 0.;
        for(int i=0; i<130; i++)
        {
            if(rez.a > 0.99)break;

            vec3 pos = ro + t*rd;
            vec2 mpv = map(pos);
            float den = clamp(mpv.x-0.3,0.,1.)*1.12;
            float dn = clamp((mpv.x + 2.),0.,3.);
            
            vec4 col = vec4(0);
            if (mpv.x > 0.6)
            {
                col = vec4(sin(vec3(5.,0.4,0.2) + mpv.y*0.1 +sin(pos.z*0.4)*0.5 + 1.8)*0.5 + 0.5,0.08);
                col *= den*den*den;
                col.rgb *= linstep(4.,-2.5, mpv.x)*2.3;
                float dif =  clamp((den - map(pos+.8).x)/9., 0.001, 1. );
                dif += clamp((den - map(pos+.35).x)/2.5, 0.001, 1. );
                col.xyz *= den*(vec3(0.005,.045,.075) + 1.5*vec3(0.033,0.07,0.03)*dif);
            }
            
            float fogC = exp(t*0.2 - 2.2);
            col.rgba += vec4(0.06,0.11,0.11, 0.1)*clamp(fogC-fogT, 0., 1.);
            fogT = fogC;
            rez = rez + col*(1. - rez.a);
            t += clamp(0.5 - dn*dn*.05, 0.09, 0.3);
        }
        return clamp(rez, 0.0, 1.0);
    }

    float getsat(vec3 c)
    {
        float mi = min(min(c.x, c.y), c.z);
        float ma = max(max(c.x, c.y), c.z);
        return (ma - mi)/(ma+ 1e-7);
    }

    vec3 iLerp(in vec3 a, in vec3 b, in float x)
    {
        vec3 ic = mix(a, b, x) + vec3(1e-6,0.,0.);
        float sd = abs(getsat(ic) - mix(getsat(a), getsat(b), x));
        vec3 dir = normalize(vec3(2.*ic.x - ic.y - ic.z, 2.*ic.y - ic.x - ic.z, 2.*ic.z - ic.y - ic.x));
        float lgt = dot(vec3(1.0), ic);
        float ff = dot(dir, normalize(ic));
        ic += 1.5*dir*sd*ff*lgt;
        return clamp(ic,0.,1.);
    }

    void main(void)
    {   
        vec2 q = vec2(fragCoord.x, 1.0 - fragCoord.y);
        vec2 p = (vec2(fragCoord.x, 1.0 - fragCoord.y) * iResolution.xy - 0.5*iResolution.xy)/iResolution.y;
        bsMo = vec2(0);
        
        float time = iTime*3.;
        vec3 ro = vec3(0,0,time);
        
        ro += vec3(sin(iTime)*0.5,sin(iTime*1.)*0.,0);
            
        float dspAmp = .85;
        ro.xy += disp(ro.z)*dspAmp;
        float tgtDst = 3.5;
        
        vec3 target = normalize(ro - vec3(disp(time + tgtDst)*dspAmp, time + tgtDst));
        ro.x -= bsMo.x*2.;
        vec3 rightdir = normalize(cross(target, vec3(0,1,0)));
        vec3 updir = normalize(cross(rightdir, target));
        rightdir = normalize(cross(updir, target));
        vec3 rd=normalize((p.x*rightdir + p.y*updir)*1. - target);
        rd.xy *= rot(-disp(time + 3.5).x*0.2 + bsMo.x);
        prm1 = smoothstep(-0.4, 0.4,sin(iTime*0.3));
        vec4 scn = render(ro, rd, time);
            
        vec3 col = scn.rgb;
        col = iLerp(col.bgr, col.rgb, clamp(1.-prm1,0.05,1.));
        
        col = pow(col, vec3(.55,0.65,0.6))*vec3(1.,.97,.9);

        col *= pow( 16.0*q.x*q.y*(1.0-q.x)*(1.0-q.y), 0.12)*0.7+0.3;
        
        // Rounded corners with bevel
        vec2 uv = fragCoord * iResolution.xy;
        float radius = 12.0;
        float bevelWidth = 5.0;
        vec2 dist = min(uv, iResolution.xy - uv);
        float cornerDist = length(max(vec2(radius) - dist, 0.0));
        float alpha = 1.0 - smoothstep(radius - 1.0, radius, cornerDist);
        
        // Bevel effect
        float edgeDist = min(min(dist.x, dist.y), cornerDist);
        float bevel = smoothstep(0.0, bevelWidth, edgeDist);
        bevel = pow(bevel, 0.8);
        
        col *= mix(1.0, 1.4, bevel);
        
        float innerGlow = smoothstep(bevelWidth + 2.0, bevelWidth, edgeDist);
        col += vec3(0.15, 0.2, 0.25) * innerGlow * 0.3;
        
        fragColor = vec4( col, alpha );
    }
)";

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
    int icon_size = 96;
    float transparency = 0.90f;
    std::set<std::string> favorites;
    std::map<std::string, int> launch_counts;
    std::map<std::string, time_t> last_launches;
    
    bool validate() {
        if (icon_size < 16 || icon_size > 256) icon_size = 96;
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
            } catch (...) {
                error_count++;
                continue;
            }
        }
        
        file.close();
        
        if (error_count > 10) {
            std::string backup_path = config_path + ".backup";
            std::rename(config_path.c_str(), backup_path.c_str());
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
    GtkWidget *gl_area;
    GtkCssProvider *css_provider;
    
    GLuint shader_program;
    GLuint vao, vbo;
    gint64 start_time;
    
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
    int morph_current_theme = 0;
    
    bool calculator_mode = false;
    bool web_search_mode = false;
    bool command_mode = false;
    
    static constexpr int LAUNCHER_WIDTH = 500;
    static constexpr int LAUNCHER_HEIGHT = 600;
    static constexpr int MARGIN_TOP = 50;
    static constexpr bool POSITION_TOP_LEFT = true;
    
    // Shader initialization
    void init_shaders() {
        // Compile vertex shader
        GLuint vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &vertex_shader_source, NULL);
        glCompileShader(vs);
        
        // Check vertex shader
        GLint success;
        glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
        if (!success) {
            char log[512];
            glGetShaderInfoLog(vs, 512, NULL, log);
            std::cerr << "Vertex shader error: " << log << std::endl;
        }
        
        // Compile fragment shader
        GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &fragment_shader_source, NULL);
        glCompileShader(fs);
        
        // Check fragment shader
        glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
        if (!success) {
            char log[512];
            glGetShaderInfoLog(fs, 512, NULL, log);
            std::cerr << "Fragment shader error: " << log << std::endl;
        }
        
        // Link program
        shader_program = glCreateProgram();
        glAttachShader(shader_program, vs);
        glAttachShader(shader_program, fs);
        glLinkProgram(shader_program);
        
        // Check program
        glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
        if (!success) {
            char log[512];
            glGetProgramInfoLog(shader_program, 512, NULL, log);
            std::cerr << "Shader program error: " << log << std::endl;
        }
        
        glDeleteShader(vs);
        glDeleteShader(fs);
        
        // Create quad
        float vertices[] = {
            -1.0f, -1.0f,
             1.0f, -1.0f,
            -1.0f,  1.0f,
             1.0f,  1.0f
        };
        
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);
    }
    
    static void on_gl_realize(GtkGLArea *area, gpointer user_data) {
        FuturisticLauncher *launcher = static_cast<FuturisticLauncher*>(user_data);
        gtk_gl_area_make_current(area);
        
        if (gtk_gl_area_get_error(area) != NULL) {
            std::cerr << "GL Area error on realize" << std::endl;
            return;
        }
        
        launcher->init_shaders();
    }
    
    static gboolean on_gl_render(GtkGLArea *area, GdkGLContext *context, gpointer user_data) {
        FuturisticLauncher *launcher = static_cast<FuturisticLauncher*>(user_data);
        
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Enable blending for transparency
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        glUseProgram(launcher->shader_program);
        
        // Calculate time
        gint64 current_time = g_get_monotonic_time();
        float time = (current_time - launcher->start_time) / 1000000.0f;
        
        // Get resolution (GTK3/4 compatible)
        int width, height;
        #if GTK_IS_VERSION_4
            width = gtk_widget_get_width(GTK_WIDGET(area));
            height = gtk_widget_get_height(GTK_WIDGET(area));
        #else
            GtkAllocation alloc;
            gtk_widget_get_allocation(GTK_WIDGET(area), &alloc);
            width = alloc.width;
            height = alloc.height;
        #endif
        
        glUniform1f(glGetUniformLocation(launcher->shader_program, "time"), time);
        glUniform2f(glGetUniformLocation(launcher->shader_program, "resolution"), width, height);
        
        glBindVertexArray(launcher->vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        
        glDisable(GL_BLEND);
        
        return TRUE;
    }
    
    static gboolean gl_tick_callback(GtkWidget *widget, GdkFrameClock *clock, gpointer data) {
        gtk_gl_area_queue_render(GTK_GL_AREA(widget));
        return G_SOURCE_CONTINUE;
    }
    
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
                score += 1 + consecutive * 5;
                consecutive++;
                pat_idx++;
            } else {
                consecutive = 0;
            }
            str_idx++;
        }
        
        if (pat_idx != pattern_lower.length()) {
            return 0;
        }
        
        if (str_lower.find(pattern_lower) != std::string::npos) {
            score += 50;
        }
        
        if (str_lower.find(pattern_lower) == 0) {
            score += 100;
        }
        
        return score;
    }
    
    std::string get_theme_css() {
        ThemeColors colors = theme_palette[config.current_theme];
        std::ostringstream css;
        
        css << R"(
            window {
                background: transparent;
            }
            
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
            
            entry {
                background: rgba(15, 15, 25, 0.85);
                color: rgba(180, 230, 255, 1.0);
                border: 2px solid rgba()" << colors.primary << R"(, 0.5);
                border-radius: 8px;
                padding: 12px 16px;
                font-size: 16px;
                font-weight: bold;
            }
            
            entry:focus {
                border: 2px solid rgba()" << colors.secondary << R"(, 0.9);
                background: rgba(20, 20, 35, 0.90);
            }
            
            #icon-box {
                background: rgba(12, 12, 20, 0.6);
                border: 1px solid rgba(40, 40, 60, 0.5);
                border-radius: 6px;
                padding: 8px;
                margin: 2px;
                transition: all 150ms cubic-bezier(0.4, 0.0, 0.2, 1);
            }
            
            #icon-box:hover {
                background: rgba(20, 50, 90, 0.7);
                border: 1px solid rgba()" << colors.primary << R"(, 0.8);
                transform: scale(1.05);
            }
            
            .selected-icon {
                background: rgba(20, 50, 90, 0.7) !important;
                border: 1px solid rgba()" << colors.secondary << R"(, 0.8) !important;
                transform: scale(1.05);
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
                background: transparent;
                border-bottom: 1px solid rgba()" << colors.primary << R"(, 0.3);
                padding: 8px;
            }
            
            #stats-label {
                color: rgba()" << colors.accent << R"(, 0.9);
                font-size: 11px;
                font-family: monospace;
            }
            
            button {
                background: rgba(15, 15, 25, 0.7);
                color: rgba(180, 230, 255, 1.0);
                border: 1px solid rgba()" << colors.primary << R"(, 0.5);
                border-radius: 6px;
                padding: 6px 12px;
                transition: all 150ms ease-in-out;
            }
            
            button:hover {
                background: rgba(20, 50, 90, 0.8);
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
    
    void apply_theme() {
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
        
        fade_timer = g_timeout_add(16, fade_timer_callback, this);
    }
    
    double calculate_expression(const std::string& expr) {
        try {
            std::string clean_expr = expr;
            clean_expr.erase(std::remove(clean_expr.begin(), clean_expr.end(), ' '), clean_expr.end());
            
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
        start_time = g_get_monotonic_time();
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

        std::sort(all_apps.begin(), all_apps.end(), 
            [](const DesktopApp& a, const DesktopApp& b) {
                if (a.is_favorite != b.is_favorite) return a.is_favorite;
                if (a.launch_count != b.launch_count) return a.launch_count > b.launch_count;
                return a.name < b.name;
            });
        
        filtered_apps = all_apps;
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
        
        if (search_text[0] == '?') {
            web_search_mode = true;
            return;
        } else if (search_text[0] == '>') {
            command_mode = true;
            return;
        }
        
        std::regex math_regex(R"(^[\d\s\+\-\*\/\(\)\.]+$)");
        if (std::regex_match(search_text, math_regex)) {
            double result = calculate_expression(search_text);
            if (!std::isnan(result)) {
                calculator_mode = true;
                return;
            }
        }

        std::vector<std::pair<DesktopApp, int>> scored_apps;
        
        for (const auto& app : all_apps) {
            int name_score = fuzzy_score(app.name, search_text);
            int comment_score = fuzzy_score(app.comment, search_text) / 2;
            int total_score = name_score + comment_score;
            
            if (total_score > 0) {
                scored_apps.push_back({app, total_score});
            }
        }
        
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
        
        const char *text = gtk_editable_get_text(GTK_EDITABLE(search_entry));
        filter_apps(text);
        update_list();
    }
    
    void execute_web_search(const std::string& query) {
        std::string search_query = query.substr(1);
        std::string url = "xdg-open 'https://www.google.com/search?q=" + search_query + "'";
        int ret = system(url.c_str());
        (void)ret;
        toggle_visibility();
    }
    
    void execute_command(const std::string& command) {
        std::string cmd = command.substr(1);
        cmd += " &";
        int ret = system(cmd.c_str());
        (void)ret;
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
        
        GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        
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
            if (event->button == 1) {
                launcher->launch_app(launcher->filtered_apps[index]);
            } else if (event->button == 3) {
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
        
        if (state & GDK_CONTROL_MASK) {
            if (keyval >= GDK_KEY_1 && keyval <= GDK_KEY_7) {
                launcher->config.current_theme = static_cast<Theme>(keyval - GDK_KEY_1);
                launcher->apply_theme();
                return TRUE;
            }
        }
        
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

        // CSS
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

        // Create overlay to stack shader background and UI
        #if GTK_IS_VERSION_4
            GtkWidget *overlay = gtk_overlay_new();
        #else
            GtkWidget *overlay = gtk_overlay_new();
        #endif
        gtk_window_set_child(GTK_WINDOW(window), overlay);
        
        // GL Area (shader background)
        gl_area = gtk_gl_area_new();
        gtk_widget_set_hexpand(gl_area, TRUE);
        gtk_widget_set_vexpand(gl_area, TRUE);
        gtk_gl_area_set_has_alpha(GTK_GL_AREA(gl_area), TRUE);
        g_signal_connect(gl_area, "realize", G_CALLBACK(on_gl_realize), this);
        g_signal_connect(gl_area, "render", G_CALLBACK(on_gl_render), this);
        gtk_widget_add_tick_callback(gl_area, gl_tick_callback, NULL, NULL);
        
        gtk_overlay_set_child(GTK_OVERLAY(overlay), gl_area);
        
        // Main UI box on top of shader
        GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_overlay_add_overlay_compat(GTK_OVERLAY(overlay), main_box);
        
        // Header
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

        // Key press
        #if GTK_IS_VERSION_4
            GtkEventController *key_controller = gtk_event_controller_key_new();
            gtk_widget_add_controller(window, key_controller);
            g_signal_connect(key_controller, "key-pressed", G_CALLBACK(on_key_press), this);
        #else
            g_signal_connect(window, "key-press-event", G_CALLBACK(on_key_press), this);
        #endif

        update_list();
        
        stats_timer = g_timeout_add_seconds(1, stats_timer_callback, this);
        update_stats();

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