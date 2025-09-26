#ifndef DISPLAY_H
#define DISPLAY_H

#include <raylib.h>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include "modern_vec3.h"

// Avoid naming conflicts between Raylib's Color and our Vec3
typedef struct RaylibColor {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} RaylibColor;

class InteractiveDisplay {
private:
    int window_width;
    int window_height;
    int render_width;
    int render_height;
    
    RenderTexture2D render_target;
    std::vector<RaylibColor> pixel_buffer;
    std::vector<Vec3> render_buffer;
    std::mutex buffer_mutex;
    
    std::atomic<bool> should_close{false};
    std::atomic<int> completed_pixels{0};
    std::atomic<int> total_pixels{0};
    std::atomic<float> render_time{0.0f};
    
    bool show_stats = true;
    bool show_controls = true;
    
public:
    InteractiveDisplay(int win_w, int win_h, int render_w, int render_h);
    ~InteractiveDisplay();
    
    bool Initialize();
    void Shutdown();
    
    void UpdatePixel(int x, int y, const Vec3& color);
    void UpdateRegion(int start_x, int start_y, int width, int height, 
                     const std::vector<Vec3>& colors);
    
    void BeginFrame();
    void EndFrame();
    void DrawUI();
    
    bool ShouldClose() const { return should_close.load() || WindowShouldClose(); }
    void RequestClose() { should_close.store(true); }
    
    void SetProgress(int completed, int total) { 
        completed_pixels.store(completed); 
        total_pixels.store(total); 
    }
    void SetRenderTime(float time) { render_time.store(time); }
    
    void ToggleStats() { show_stats = !show_stats; }
    void ToggleControls() { show_controls = !show_controls; }
    
    // Input handling
    bool IsKeyPressed(int key) const { return ::IsKeyPressed(key); }
    bool IsKeyDown(int key) const { return ::IsKeyDown(key); }
    Vector2 GetMousePosition() const { return ::GetMousePosition(); }
    bool IsMouseButtonPressed(int button) const { return ::IsMouseButtonPressed(button); }
};

// Progressive rendering interface
class ProgressiveRenderer {
private:
    std::unique_ptr<InteractiveDisplay> display;
    std::atomic<bool> pause_rendering{false};
    std::atomic<bool> stop_rendering{false};
    
    // Rendering parameters
    int samples_per_pixel = 10;
    int max_depth = 50;
    
public:
    ProgressiveRenderer(int window_width, int window_height, 
                       int render_width, int render_height);
    ~ProgressiveRenderer();
    
    bool Initialize();
    void Shutdown();
    
    void SetSamplesPerPixel(int samples) { samples_per_pixel = samples; }
    void SetMaxDepth(int depth) { max_depth = depth; }
    
    void PauseRendering() { pause_rendering.store(true); }
    void ResumeRendering() { pause_rendering.store(false); }
    void StopRendering() { stop_rendering.store(true); }
    
    bool IsPaused() const { return pause_rendering.load(); }
    bool ShouldStop() const { return stop_rendering.load(); }
    bool ShouldClose() const { return display->ShouldClose(); }
    
    InteractiveDisplay* GetDisplay() { return display.get(); }
    
    void HandleInput();
};

#endif // DISPLAY_H