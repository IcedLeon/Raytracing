#include <raylib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <cmath>
#include <random>

// Simple 3D vector class to avoid conflicts
struct Vec3D {
    float x, y, z;
    Vec3D(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
    Vec3D operator+(const Vec3D& v) const { return Vec3D(x + v.x, y + v.y, z + v.z); }
    Vec3D operator-(const Vec3D& v) const { return Vec3D(x - v.x, y - v.y, z - v.z); }
    Vec3D operator*(float t) const { return Vec3D(x * t, y * t, z * t); }
    Vec3D operator/(float t) const { return *this * (1.0f / t); }
    float dot(const Vec3D& v) const { return x * v.x + y * v.y + z * v.z; }
    float length() const { return sqrtf(x * x + y * y + z * z); }
    Vec3D normalize() const { float l = length(); return l > 0 ? *this / l : Vec3D(); }
};

// Simple ray
struct RayD {
    Vec3D origin, direction;
    RayD(const Vec3D& o, const Vec3D& d) : origin(o), direction(d) {}
    Vec3D at(float t) const { return origin + direction * t; }
};

// Simple sphere
struct SphereD {
    Vec3D center;
    float radius;
    Vec3D color;
    
    SphereD(const Vec3D& c, float r, const Vec3D& col) : center(c), radius(r), color(col) {}
    
    bool hit(const RayD& ray, float t_min, float t_max, float& t, Vec3D& normal) const {
        Vec3D oc = ray.origin - center;
        float a = ray.direction.dot(ray.direction);
        float b = 2.0f * oc.dot(ray.direction);
        float c = oc.dot(oc) - radius * radius;
        float discriminant = b * b - 4 * a * c;
        
        if (discriminant < 0) return false;
        
        float sqrt_d = sqrtf(discriminant);
        float t1 = (-b - sqrt_d) / (2.0f * a);
        float t2 = (-b + sqrt_d) / (2.0f * a);
        
        float hit_t = (t1 > t_min && t1 < t_max) ? t1 : 
                     (t2 > t_min && t2 < t_max) ? t2 : -1.0f;
        
        if (hit_t > 0) {
            t = hit_t;
            Vec3D hit_point = ray.at(hit_t);
            normal = (hit_point - center).normalize();
            return true;
        }
        return false;
    }
};

// Simple scene
class Scene {
private:
    std::vector<SphereD> spheres;
    mutable std::mt19937 rng;
    mutable std::uniform_real_distribution<float> dist{0.0f, 1.0f};
    
public:
    Scene() : rng(std::random_device{}()) {
        // Ground sphere
        spheres.emplace_back(Vec3D(0, -100.5f, -1), 100.0f, Vec3D(0.5f, 0.5f, 0.5f));
        
        // Three main spheres
        spheres.emplace_back(Vec3D(0, 0, -1), 0.5f, Vec3D(0.7f, 0.3f, 0.3f));
        spheres.emplace_back(Vec3D(-1, 0, -1), 0.5f, Vec3D(0.3f, 0.7f, 0.3f));
        spheres.emplace_back(Vec3D(1, 0, -1), 0.5f, Vec3D(0.3f, 0.3f, 0.7f));
        
        // Some random small spheres
        for (int i = 0; i < 10; ++i) {
            Vec3D pos(dist(rng) * 4 - 2, -0.3f, -dist(rng) * 2 - 0.5f);
            Vec3D col(dist(rng), dist(rng), dist(rng));
            spheres.emplace_back(pos, 0.1f + dist(rng) * 0.2f, col);
        }
    }
    
    Vec3D rayColor(const RayD& ray, int depth = 0) const {
        if (depth >= 4) return Vec3D(0, 0, 0);
        
        float closest_t = 1000.0f;
        Vec3D hit_normal;
        Vec3D hit_color;
        bool hit_anything = false;
        
        for (const auto& sphere : spheres) {
            float t;
            Vec3D normal;
            if (sphere.hit(ray, 0.001f, closest_t, t, normal)) {
                closest_t = t;
                hit_normal = normal;
                hit_color = sphere.color;
                hit_anything = true;
            }
        }
        
        if (hit_anything) {
            // Simple diffuse shading
            Vec3D hit_point = ray.at(closest_t);
            Vec3D random_dir(dist(rng) - 0.5f, dist(rng) - 0.5f, dist(rng) - 0.5f);
            Vec3D target = hit_point + hit_normal + random_dir.normalize() * 0.5f;
            RayD scatter_ray(hit_point, (target - hit_point).normalize());
            return hit_color * 0.5f + rayColor(scatter_ray, depth + 1) * 0.5f;
        }
        
        // Sky gradient
        Vec3D unit_dir = ray.direction.normalize();
        float t = 0.5f * (unit_dir.y + 1.0f);
        return Vec3D(1, 1, 1) * (1.0f - t) + Vec3D(0.5f, 0.7f, 1.0f) * t;
    }
    
    float random() const { return dist(rng); }
};

// Interactive raytracer display
class InteractiveRaytracer {
private:
    int window_width, window_height;
    int render_width, render_height;
    
    RenderTexture2D render_texture;
    std::vector<Color> pixel_buffer;
    std::mutex pixel_mutex;
    
    Scene scene;
    
    std::atomic<bool> rendering{false};
    std::atomic<bool> should_stop{false};
    std::atomic<int> completed_pixels{0};
    std::atomic<int> total_pixels{0};
    std::atomic<float> render_time{0.0f};
    
    int samples_per_pixel = 4;
    
public:
    InteractiveRaytracer(int win_w, int win_h, int render_w, int render_h)
        : window_width(win_w), window_height(win_h), 
          render_width(render_w), render_height(render_h) {
        pixel_buffer.resize(render_width * render_height, BLACK);
        total_pixels = render_width * render_height;
    }
    
    bool Initialize() {
        InitWindow(window_width, window_height, "Interactive Raytracing - Real-time Progress");
        SetTargetFPS(60);
        
        if (!IsWindowReady()) {
            std::cerr << "Failed to initialize window\n";
            return false;
        }
        
        render_texture = LoadRenderTexture(render_width, render_height);
        if (render_texture.id == 0) {
            std::cerr << "Failed to create render texture\n";
            CloseWindow();
            return false;
        }
        
        return true;
    }
    
    void Shutdown() {
        should_stop = true;
        if (render_texture.id != 0) {
            UnloadRenderTexture(render_texture);
        }
        if (IsWindowReady()) {
            CloseWindow();
        }
    }
    
    void UpdatePixel(int x, int y, const Vec3D& color) {
        if (x < 0 || x >= render_width || y < 0 || y >= render_height) return;
        
        std::lock_guard<std::mutex> lock(pixel_mutex);
        int index = y * render_width + x;
        
        // Gamma correction and clamping
        Vec3D gamma_corrected(sqrtf(color.x), sqrtf(color.y), sqrtf(color.z));
        pixel_buffer[index] = Color{
            (unsigned char)(std::min(255.0f, gamma_corrected.x * 255.0f)),
            (unsigned char)(std::min(255.0f, gamma_corrected.y * 255.0f)),
            (unsigned char)(std::min(255.0f, gamma_corrected.z * 255.0f)),
            255
        };
    }
    
    void StartRendering() {
        if (rendering.exchange(true)) return; // Already rendering
        
        should_stop = false;
        completed_pixels = 0;
        
        std::thread render_thread([this]() {
            auto start_time = std::chrono::high_resolution_clock::now();
            
            for (int j = render_height - 1; j >= 0 && !should_stop.load(); --j) {
                for (int i = 0; i < render_width && !should_stop.load(); ++i) {
                    Vec3D color(0, 0, 0);
                    
                    // Multiple samples per pixel
                    for (int s = 0; s < samples_per_pixel; ++s) {
                        float u = (i + scene.random()) / render_width;
                        float v = (j + scene.random()) / render_height;
                        
                        // Simple camera
                        Vec3D origin(0, 0, 0);
                        Vec3D lower_left(-2, -1.5, -1);
                        Vec3D horizontal(4, 0, 0);
                        Vec3D vertical(0, 3, 0);
                        Vec3D direction = lower_left + horizontal * u + vertical * v - origin;
                        
                        RayD ray(origin, direction.normalize());
                        color = color + scene.rayColor(ray);
                    }
                    
                    color = color / samples_per_pixel;
                    UpdatePixel(i, j, color);
                    completed_pixels++;
                    
                    // Small delay every few pixels to allow UI updates
                    if (completed_pixels % 100 == 0) {
                        std::this_thread::sleep_for(std::chrono::microseconds(100));
                    }
                }
            }
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            render_time = duration.count() / 1000.0f;
            
            rendering = false;
        });
        
        render_thread.detach();
    }
    
    void HandleInput() {
        if (IsKeyPressed(KEY_SPACE)) {
            if (!rendering.load()) {
                StartRendering();
            }
        }
        
        if (IsKeyPressed(KEY_S)) {
            SaveImage();
        }
        
        if (IsKeyPressed(KEY_R)) {
            should_stop = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (!rendering.load()) {
                std::fill(pixel_buffer.begin(), pixel_buffer.end(), BLACK);
                completed_pixels = 0;
            }
        }
        
        if (IsKeyPressed(KEY_UP) && samples_per_pixel < 16) {
            samples_per_pixel++;
        }
        if (IsKeyPressed(KEY_DOWN) && samples_per_pixel > 1) {
            samples_per_pixel--;
        }
    }
    
    void SaveImage() {
        std::cout << "Saving image...\n";
        
        // Export as PPM
        std::ofstream file("raylib_output.ppm");
        file << "P3\n" << render_width << " " << render_height << "\n255\n";
        
        for (int j = render_height - 1; j >= 0; --j) {
            for (int i = 0; i < render_width; ++i) {
                int index = j * render_width + i;
                const Color& c = pixel_buffer[index];
                file << (int)c.r << " " << (int)c.g << " " << (int)c.b << "\n";
            }
        }
        file.close();
        std::cout << "Saved to raylib_output.ppm\n";
    }
    
    void Draw() {
        BeginDrawing();
        ClearBackground(BLACK);
        
        // Update render texture
        {
            std::lock_guard<std::mutex> lock(pixel_mutex);
            BeginTextureMode(render_texture);
            for (int y = 0; y < render_height; ++y) {
                for (int x = 0; x < render_width; ++x) {
                    int index = y * render_width + x;
                    DrawPixel(x, render_height - 1 - y, pixel_buffer[index]);
                }
            }
            EndTextureMode();
        }
        
        // Scale render texture to fit window
        float scale_x = (float)window_width / render_width;
        float scale_y = (float)window_height / render_height;
        float scale = std::min(scale_x, scale_y) * 0.8f; // Leave room for UI
        
        int display_width = (int)(render_width * scale);
        int display_height = (int)(render_height * scale);
        int offset_x = (window_width - display_width) / 2;
        int offset_y = 50; // Leave room for UI at top
        
        // Draw render texture
        DrawTexturePro(render_texture.texture,
                       {0, 0, (float)render_width, -(float)render_height},
                       {(float)offset_x, (float)offset_y, (float)display_width, (float)display_height},
                       {0, 0}, 0, WHITE);
        
        // Draw UI
        DrawUI();
        
        EndDrawing();
    }
    
    void DrawUI() {
        int completed = completed_pixels.load();
        int total = total_pixels.load();
        float progress = total > 0 ? (float)completed / total : 0.0f;
        float time = render_time.load();
        bool is_rendering = rendering.load();
        
        // Title
        DrawText("Interactive Raytracing", 20, 10, 24, WHITE);
        
        // Status
        const char* status = is_rendering ? "Rendering..." : "Ready";
        DrawText(status, 300, 15, 20, is_rendering ? GREEN : GRAY);
        
        // Progress info
        DrawText(TextFormat("Resolution: %dx%d", render_width, render_height), 20, 40, 16, LIGHTGRAY);
        DrawText(TextFormat("Samples/pixel: %d", samples_per_pixel), 200, 40, 16, LIGHTGRAY);
        DrawText(TextFormat("Progress: %d/%d (%.1f%%)", completed, total, progress * 100.0f), 350, 40, 16, LIGHTGRAY);
        
        if (time > 0) {
            DrawText(TextFormat("Render time: %.2fs", time), 550, 40, 16, LIGHTGRAY);
        }
        
        // Progress bar
        int bar_width = window_width - 40;
        DrawRectangle(20, window_height - 80, bar_width, 20, DARKGRAY);
        DrawRectangle(20, window_height - 80, (int)(bar_width * progress), 20, GREEN);
        
        // Controls
        DrawText("Controls:", 20, window_height - 50, 16, WHITE);
        DrawText("SPACE - Start Rendering", 20, window_height - 30, 14, LIGHTGRAY);
        DrawText("S - Save Image", 200, window_height - 30, 14, LIGHTGRAY);
        DrawText("R - Reset", 320, window_height - 30, 14, LIGHTGRAY);
        DrawText("UP/DOWN - Adjust Quality", 400, window_height - 30, 14, LIGHTGRAY);
        DrawText("ESC - Exit", 600, window_height - 30, 14, LIGHTGRAY);
    }
    
    bool ShouldClose() const {
        return WindowShouldClose();
    }
};

int main() {
    const int window_width = 1000;
    const int window_height = 700;
    const int render_width = 400;
    const int render_height = 300;
    
    InteractiveRaytracer raytracer(window_width, window_height, render_width, render_height);
    
    if (!raytracer.Initialize()) {
        std::cerr << "Failed to initialize raytracer\n";
        return -1;
    }
    
    std::cout << "Interactive Raytracer started!\n";
    std::cout << "Window: " << window_width << "x" << window_height << "\n";
    std::cout << "Render: " << render_width << "x" << render_height << "\n";
    std::cout << "Press SPACE to start rendering\n";
    
    // Main loop
    while (!raytracer.ShouldClose()) {
        raytracer.HandleInput();
        raytracer.Draw();
    }
    
    raytracer.Shutdown();
    
    std::cout << "Raytracer shutdown complete\n";
    return 0;
}