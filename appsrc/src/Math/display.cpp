#include "appsrc/include/Math/display.h"
#include <iostream>
#include <cstdio>
#include <algorithm>

InteractiveDisplay::InteractiveDisplay(int win_w, int win_h, int render_w, int render_h)
    : window_width(win_w), window_height(win_h), 
      render_width(render_w), render_height(render_h) {
    pixel_buffer.resize(render_width * render_height, {0, 0, 0, 255});
    render_buffer.resize(render_width * render_height, Vec3(0.0f, 0.0f, 0.0f));
}

InteractiveDisplay::~InteractiveDisplay() {
    Shutdown();
}

bool InteractiveDisplay::Initialize() {
    InitWindow(window_width, window_height, "Raytracing - Interactive Renderer");
    SetTargetFPS(60);
    
    if (!IsWindowReady()) {
        std::cerr << "Failed to initialize window" << std::endl;
        return false;
    }
    
    render_target = LoadRenderTexture(render_width, render_height);
    if (render_target.id == 0) {
        std::cerr << "Failed to create render texture" << std::endl;
        CloseWindow();
        return false;
    }
    
    return true;
}

void InteractiveDisplay::Shutdown() {
    if (render_target.id != 0) {
        UnloadRenderTexture(render_target);
        render_target.id = 0;
    }
    if (IsWindowReady()) {
        CloseWindow();
    }
}

void InteractiveDisplay::UpdatePixel(int x, int y, const Vec3& color) {
    if (x < 0 || x >= render_width || y < 0 || y >= render_height) return;
    
    std::lock_guard<std::mutex> lock(buffer_mutex);
    int index = y * render_width + x;
    render_buffer[index] = color;
    
    // Convert to Raylib color (gamma corrected)
    Vec3 gamma_corrected = Vec3(sqrtf(color.GetX()), sqrtf(color.GetY()), sqrtf(color.GetZ()));
    pixel_buffer[index] = RaylibColor{
        (unsigned char)(std::min(255.0f, gamma_corrected.GetX() * 255.0f)),
        (unsigned char)(std::min(255.0f, gamma_corrected.GetY() * 255.0f)),
        (unsigned char)(std::min(255.0f, gamma_corrected.GetZ() * 255.0f)),
        255
    };
}

void InteractiveDisplay::UpdateRegion(int start_x, int start_y, int width, int height, 
                                    const std::vector<Vec3>& colors) {
    std::lock_guard<std::mutex> lock(buffer_mutex);
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int pixel_x = start_x + x;
            int pixel_y = start_y + y;
            
            if (pixel_x >= render_width || pixel_y >= render_height) continue;
            
            int buffer_index = y * width + x;
            int pixel_index = pixel_y * render_width + pixel_x;
            
            if (buffer_index < colors.size()) {
                const Vec3& color = colors[buffer_index];
                render_buffer[pixel_index] = color;
                
                // Convert to Raylib color (gamma corrected)
                Vec3 gamma_corrected = Vec3(sqrtf(color.GetX()), sqrtf(color.GetY()), sqrtf(color.GetZ()));
                pixel_buffer[pixel_index] = RaylibColor{
                    (unsigned char)(std::min(255.0f, gamma_corrected.GetX() * 255.0f)),
                    (unsigned char)(std::min(255.0f, gamma_corrected.GetY() * 255.0f)),
                    (unsigned char)(std::min(255.0f, gamma_corrected.GetZ() * 255.0f)),
                    255
                };
            }
        }
    }
}

void InteractiveDisplay::BeginFrame() {
    BeginDrawing();
    ClearBackground(BLACK);
    
    // Update render texture with current pixel buffer
    {
        std::lock_guard<std::mutex> lock(buffer_mutex);
        
        BeginTextureMode(render_target);
        for (int y = 0; y < render_height; ++y) {
            for (int x = 0; x < render_width; ++x) {
                int index = y * render_width + x;
                const RaylibColor& pixel = pixel_buffer[index];
                DrawPixel(x, render_height - 1 - y, 
                         Color{pixel.r, pixel.g, pixel.b, pixel.a}); // Flip Y
            }
        }
        EndTextureMode();
    }
    
    // Calculate scaling to fit window
    float scale_x = (float)window_width / render_width;
    float scale_y = (float)window_height / render_height;
    float scale = std::min(scale_x, scale_y);
    
    int display_width = (int)(render_width * scale);
    int display_height = (int)(render_height * scale);
    int offset_x = (window_width - display_width) / 2;
    int offset_y = (window_height - display_height) / 2;
    
    // Draw the render texture
    DrawTexturePro(render_target.texture, 
                   {0, 0, (float)render_width, -(float)render_height},
                   {(float)offset_x, (float)offset_y, (float)display_width, (float)display_height},
                   {0, 0}, 0, WHITE);
}

void InteractiveDisplay::EndFrame() {
    DrawUI();
    EndDrawing();
}

void InteractiveDisplay::DrawUI() {
    if (show_stats) {
        int total = total_pixels.load();
        int completed = completed_pixels.load();
        float time = render_time.load();
        
        float progress = total > 0 ? (float)completed / total : 0.0f;
        
        // Draw semi-transparent background
        DrawRectangle(10, 10, 300, 120, {0, 0, 0, 180});
        
        // Draw stats
        DrawText("Raytracing Progress", 20, 20, 20, WHITE);
        DrawText(TextFormat("Resolution: %dx%d", render_width, render_height), 20, 45, 16, LIGHTGRAY);
        DrawText(TextFormat("Progress: %d/%d (%.1f%%)", completed, total, progress * 100.0f), 20, 65, 16, LIGHTGRAY);
        DrawText(TextFormat("Time: %.2fs", time), 20, 85, 16, LIGHTGRAY);
        
        // Draw progress bar
        DrawRectangle(20, 105, 260, 10, DARKGRAY);
        DrawRectangle(20, 105, (int)(260 * progress), 10, GREEN);
    }
    
    if (show_controls) {
        // Draw controls in bottom right
        const char* controls[] = {
            "Controls:",
            "S - Toggle Stats",
            "C - Toggle Controls", 
            "P - Pause/Resume",
            "ESC - Exit"
        };
        
        int control_count = 5;
        int line_height = 16;
        int panel_height = control_count * line_height + 20;
        int panel_width = 180;
        
        DrawRectangle(window_width - panel_width - 10, window_height - panel_height - 10, 
                     panel_width, panel_height, {0, 0, 0, 180});
        
        for (int i = 0; i < control_count; ++i) {
            Color color = (i == 0) ? WHITE : LIGHTGRAY;
            DrawText(controls[i], window_width - panel_width, 
                    window_height - panel_height + i * line_height, 14, color);
        }
    }
}

// ProgressiveRenderer implementation
ProgressiveRenderer::ProgressiveRenderer(int window_width, int window_height, 
                                       int render_width, int render_height) {
    display = std::make_unique<InteractiveDisplay>(window_width, window_height, 
                                                  render_width, render_height);
}

ProgressiveRenderer::~ProgressiveRenderer() {
    Shutdown();
}

bool ProgressiveRenderer::Initialize() {
    return display->Initialize();
}

void ProgressiveRenderer::Shutdown() {
    if (display) {
        display->Shutdown();
    }
}

void ProgressiveRenderer::HandleInput() {
    if (display->IsKeyPressed(KEY_S)) {
        display->ToggleStats();
    }
    if (display->IsKeyPressed(KEY_C)) {
        display->ToggleControls();
    }
    if (display->IsKeyPressed(KEY_P)) {
        if (IsPaused()) {
            ResumeRendering();
        } else {
            PauseRendering();
        }
    }
    if (display->IsKeyPressed(KEY_ESCAPE)) {
        StopRendering();
    }
}