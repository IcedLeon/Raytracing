#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <algorithm>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>

// Include raylib first to avoid conflicts
#include <raylib.h>

// Then our math includes
#include "appsrc/include/Math/sphere.h"
#include "appsrc/include/Math/modern_vec3.h"
#include "appsrc/include/Math/hittablelist.h"
#include "appsrc/include/Math/camera.h"
#include "appsrc/include/Math/material.h"
#include "appsrc/include/Math/performance.h"

// Simple display class that avoids naming conflicts
class SimpleDisplay {
private:
    int window_width, window_height;
    int render_width, render_height;
    RenderTexture2D render_target;
    std::vector<::Color> pixel_buffer; // Use explicit :: to reference Raylib's Color
    std::mutex buffer_mutex;
    std::atomic<int> completed_pixels{0};
    std::atomic<int> total_pixels{0};
    std::atomic<float> render_time{0.0f};
    
public:
    SimpleDisplay(int win_w, int win_h, int render_w, int render_h)
        : window_width(win_w), window_height(win_h), 
          render_width(render_w), render_height(render_h) {
        pixel_buffer.resize(render_width * render_height, BLACK);
    }
    
    bool Initialize() {
        InitWindow(window_width, window_height, "Raytracing - Interactive Renderer");
        SetTargetFPS(60);
        
        if (!IsWindowReady()) {
            std::cerr << "Failed to initialize window" << std::endl;
            return false;
        }
        
        render_target = LoadRenderTexture(render_width, render_height);
        return render_target.id != 0;
    }
    
    void Shutdown() {
        if (render_target.id != 0) {
            UnloadRenderTexture(render_target);
            render_target.id = 0;
        }
        if (IsWindowReady()) {
            CloseWindow();
        }
    }
    
    void UpdatePixel(int x, int y, const Vec3& color) {
        if (x < 0 || x >= render_width || y < 0 || y >= render_height) return;
        
        std::lock_guard<std::mutex> lock(buffer_mutex);
        int index = y * render_width + x;
        
        // Convert Vec3 to Raylib Color with gamma correction
        Vec3 gamma_corrected = Vec3(sqrtf(color.GetX()), sqrtf(color.GetY()), sqrtf(color.GetZ()));
        pixel_buffer[index] = ::Color{
            (unsigned char)(std::min(255.0f, gamma_corrected.GetX() * 255.0f)),
            (unsigned char)(std::min(255.0f, gamma_corrected.GetY() * 255.0f)),
            (unsigned char)(std::min(255.0f, gamma_corrected.GetZ() * 255.0f)),
            255
        };
    }
    
    void BeginFrame() {
        BeginDrawing();
        ClearBackground(BLACK);
        
        // Update render texture
        {
            std::lock_guard<std::mutex> lock(buffer_mutex);
            BeginTextureMode(render_target);
            for (int y = 0; y < render_height; ++y) {
                for (int x = 0; x < render_width; ++x) {
                    int index = y * render_width + x;
                    DrawPixel(x, render_height - 1 - y, pixel_buffer[index]);
                }
            }
            EndTextureMode();
        }
        
        // Scale to fit window
        float scale_x = (float)window_width / render_width;
        float scale_y = (float)window_height / render_height;
        float scale = std::min(scale_x, scale_y);
        
        int display_width = (int)(render_width * scale);
        int display_height = (int)(render_height * scale);
        int offset_x = (window_width - display_width) / 2;
        int offset_y = (window_height - display_height) / 2;
        
        DrawTexturePro(render_target.texture, 
                       {0, 0, (float)render_width, -(float)render_height},
                       {(float)offset_x, (float)offset_y, (float)display_width, (float)display_height},
                       {0, 0}, 0, WHITE);
    }
    
    void EndFrame() {
        // Draw UI
        int total = total_pixels.load();
        int completed = completed_pixels.load();
        float time = render_time.load();
        float progress = total > 0 ? (float)completed / total : 0.0f;
        
        // Semi-transparent background for stats
        DrawRectangle(10, 10, 300, 120, {0, 0, 0, 180});
        DrawText("Raytracing Progress", 20, 20, 20, WHITE);
        DrawText(TextFormat("Resolution: %dx%d", render_width, render_height), 20, 45, 16, LIGHTGRAY);
        DrawText(TextFormat("Progress: %d/%d (%.1f%%)", completed, total, progress * 100.0f), 20, 65, 16, LIGHTGRAY);
        DrawText(TextFormat("Time: %.2fs", time), 20, 85, 16, LIGHTGRAY);
        
        // Progress bar
        DrawRectangle(20, 105, 260, 10, DARKGRAY);
        DrawRectangle(20, 105, (int)(260 * progress), 10, GREEN);
        
        // Controls
        DrawRectangle(window_width - 180, window_height - 80, 170, 70, {0, 0, 0, 180});
        DrawText("ESC - Exit", window_width - 170, window_height - 70, 14, LIGHTGRAY);
        DrawText("P - Pause/Resume", window_width - 170, window_height - 55, 14, LIGHTGRAY);
        DrawText("S - Save Image", window_width - 170, window_height - 40, 14, LIGHTGRAY);
        
        EndDrawing();
    }
    
    bool ShouldClose() const { return WindowShouldClose(); }
    bool IsKeyPressed(int key) const { return ::IsKeyPressed(key); }
    
    void SetProgress(int completed, int total) { 
        completed_pixels.store(completed); 
        total_pixels.store(total); 
    }
    void SetRenderTime(float time) { render_time.store(time); }
};

// Optimized color function for performance
Vec3 ColorFast(const RayMath::Ray &ray, Hittable *world, int depth, int max_depth = 8) {
    HitRecord rec;
    
    if (depth >= max_depth) return Vec3(0.0f, 0.0f, 0.0f);
    
    if (world->Hit(ray, 0.001f, FLT_MAX, rec)) {
        Vec3 target = rec.m_oPoint + rec.m_oNormal + Vec3(FastRandom::random(), FastRandom::random(), FastRandom::random());
        return 0.5f * ColorFast(RayMath::Ray(rec.m_oPoint, target - rec.m_oPoint), world, depth + 1, max_depth);
    }
    
    // Sky gradient
    Vec3 unit_direction = Unit_Vector(ray.Direction());
    float t = 0.5f * (unit_direction.GetY() + 1.0f);
    return (1.0f - t) * Vec3(1.0f, 1.0f, 1.0f) + t * Vec3(0.5f, 0.7f, 1.0f);
}

// Create smaller scene for better performance
Hittable *CreateFastScene() {
    int n = 20; // Much smaller scene
    Hittable **list = new Hittable*[n+1];
    
    list[0] = new Sphere(Vec3(0.0f, -1000.0f, 0.0f), 1000.0f, new Lambertian(Vec3(0.5f, 0.5f, 0.5f)));
    
    int i = 1;
    for (int a = -2; a < 3; a++) {
        for (int b = -2; b < 3; b++) {
            if (i >= n) break;
            
            float choose_mat = FastRandom::random();
            Vec3 center(a + 0.9f * FastRandom::random(), 0.2f, b + 0.9f * FastRandom::random());
            
            if ((center - Vec3(4.0f, 0.2f, 0.0f)).Length() > 0.9f) {
                if (choose_mat < 0.8f) {
                    list[i++] = new Sphere(center, 0.2f, 
                        new Lambertian(Vec3(FastRandom::random() * FastRandom::random(),
                                          FastRandom::random() * FastRandom::random(),
                                          FastRandom::random() * FastRandom::random())));
                } else {
                    list[i++] = new Sphere(center, 0.2f,
                        new Metal(Vec3(0.5f * (1.0f + FastRandom::random()),
                                     0.5f * (1.0f + FastRandom::random()),
                                     0.5f * (1.0f + FastRandom::random())),
                                0.5f * FastRandom::random()));
                }
            }
        }
        if (i >= n) break;
    }

    list[i++] = new Sphere(Vec3(0.0f, 1.0f, 0.0f), 1.0f, new Dielectric(1.5f));
    list[i++] = new Sphere(Vec3(-4.0f, 1.0f, 0.0f), 1.0f, new Lambertian(Vec3(0.4f, 0.2f, 0.1f)));
    list[i++] = new Sphere(Vec3(4.0f, 1.0f, 0.0f), 1.0f, new Metal(Vec3(0.7f, 0.6f, 0.5f), 0.0f));

    return new HittableList(list, i);
}

int main() {
    // Performance-optimized parameters
    int nx = 400;  // Much smaller resolution for real-time
    int ny = 300;
    int ns = 2;    // Fewer samples for speed
    
    int window_width = 1000;
    int window_height = 800;

    std::cout << "=== Interactive Raytracer (Simple) ===\n";
    std::cout << "Render Resolution: " << nx << "x" << ny << "\n";
    std::cout << "Window Size: " << window_width << "x" << window_height << "\n";
    std::cout << "Samples per pixel: " << ns << "\n";

    SimpleDisplay display(window_width, window_height, nx, ny);
    if (!display.Initialize()) {
        std::cerr << "Failed to initialize display\n";
        return -1;
    }

    PerformanceTimer total_timer;
    total_timer.begin();

    std::vector<Vec3> pixels(nx * ny);
    std::unique_ptr<Hittable> world(CreateFastScene());
    
    Vec3 lookFrom(8.0f, 2.0f, 3.0f);
    Vec3 lookAt(0.0f, 0.0f, 0.0f);
    float distToFocus = 8.0f;
    float aperture = 0.0f;

    Camera camera(lookFrom, lookAt, Vec3(0.0f, 1.0f, 0.0f), 
                  30.0f, float(nx) / float(ny), aperture, distToFocus);

    std::atomic<bool> rendering_complete{false};
    std::atomic<bool> pause_rendering{false};
    
    // Start rendering in background thread
    std::thread render_thread([&]() {
        int total_pixels = nx * ny;
        int completed = 0;
        
        for (int j = ny - 1; j >= 0 && !rendering_complete.load(); --j) {
            for (int i = 0; i < nx && !rendering_complete.load(); ++i) {
                while (pause_rendering.load() && !rendering_complete.load()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
                
                Vec3 col(0.0f, 0.0f, 0.0f);
                
                for (int s = 0; s < ns; ++s) {
                    float u = float(i + FastRandom::random()) / float(nx);
                    float v = float(j + FastRandom::random()) / float(ny);

                    RayMath::Ray ray = camera.GetRay(u, v);
                    col += ColorFast(ray, world.get(), 0);
                }

                col /= float(ns);
                col = Vec3(sqrtf(col[0]), sqrtf(col[1]), sqrtf(col[2]));

                pixels[j * nx + i] = col;
                display.UpdatePixel(i, j, col);
                
                completed++;
                display.SetProgress(completed, total_pixels);
                
                // Small delay to allow UI updates
                if (completed % 10 == 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
        }
        
        rendering_complete.store(true);
    });

    std::cout << "Rendering started. Press P to pause, S to save, ESC to exit.\n";

    // Main display loop
    while (!display.ShouldClose() && !rendering_complete.load()) {
        // Handle input
        if (display.IsKeyPressed(KEY_P)) {
            pause_rendering.store(!pause_rendering.load());
            std::cout << (pause_rendering.load() ? "Paused" : "Resumed") << " rendering\n";
        }
        
        if (display.IsKeyPressed(KEY_S)) {
            std::cout << "Saving image...\n";
            std::ofstream output_file("output_interactive_simple.ppm");
            output_file << "P3\n" << nx << " " << ny << "\n255\n";
            for (int j = ny - 1; j >= 0; --j) {
                for (int i = 0; i < nx; ++i) {
                    const Vec3 &col = pixels[j * nx + i];
                    int ir = int(255.99 * col[0]);
                    int ig = int(255.99 * col[1]);
                    int ib = int(255.99 * col[2]);
                    output_file << ir << " " << ig << " " << ib << "\n";
                }
            }
            output_file.close();
            std::cout << "Saved to output_interactive_simple.ppm\n";
        }
        
        float elapsed_time = total_timer.elapsed_ms() / 1000.0f;
        display.SetRenderTime(elapsed_time);
        
        display.BeginFrame();
        display.EndFrame();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    }

    // Clean shutdown
    rendering_complete.store(true);
    if (render_thread.joinable()) {
        render_thread.join();
    }

    std::cout << "\nTotal render time: " << (total_timer.elapsed_ms() / 1000.0f) << " seconds\n";
    
    display.Shutdown();
    return 0;
}