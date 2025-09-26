#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <algorithm>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>

#ifdef USE_OPENMP
#include <omp.h>
#endif

#include "appsrc/include/Math/sphere.h"
#include "appsrc/include/Math/modern_sphere.h"
#include "appsrc/include/Math/modern_vec3.h"
#include "appsrc/include/Math/simd_batch.h"
#include "appsrc/include/Math/hittablelist.h"
#include "appsrc/include/Math/camera.h"
#include "appsrc/include/Math/material.h"
#include "appsrc/include/Math/performance.h"
#include "appsrc/include/Math/display.h"

// Fast random number generator
class FastRandom {
private:
    static thread_local uint32_t state;
public:
    static float random() {
        state = state * 1664525u + 1013904223u;
        return (state >> 8) * (1.0f / 16777216.0f);
    }
};

thread_local uint32_t FastRandom::state = 1;

// Optimized color function with reduced recursion depth for better performance
Vec3 ColorOptimized(const Ray &ray, Hittable *world, int depth, int max_depth = 10) {
    HitRecord rec;
    
    if (depth >= max_depth) return Vec3(0.0f, 0.0f, 0.0f);
    
    if (world->Hit(ray, 0.001f, FLT_MAX, rec)) {
        Vec3 target = rec.p + rec.normal + Vec3(FastRandom::random(), FastRandom::random(), FastRandom::random());
        return 0.5f * ColorOptimized(Ray(rec.p, target - rec.p), world, depth + 1, max_depth);
    }
    
    // Sky gradient
    Vec3 unit_direction = UnitVector(ray.Direction());
    float t = 0.5f * (unit_direction.GetY() + 1.0f);
    return (1.0f - t) * Vec3(1.0f, 1.0f, 1.0f) + t * Vec3(0.5f, 0.7f, 1.0f);
}

// Create an optimized smaller scene for better performance
Hittable *CreateOptimizedScene() {
    int n = 50; // Reduced from typical random scene
    Hittable **list = new Hittable*[n+1];
    
    list[0] = new Sphere(Vec3(0.0f, -1000.0f, 0.0f), 1000.0f, new Lambertian(Vec3(0.5f, 0.5f, 0.5f)));
    
    int i = 1;
    for (int a = -5; a < 5; a++) {
        for (int b = -5; b < 5; b++) {
            if (i >= n) break;
            
            float choose_mat = FastRandom::random();
            Vec3 center(a + 0.9f * FastRandom::random(), 0.2f, b + 0.9f * FastRandom::random());
            
            if ((center - Vec3(4.0f, 0.2f, 0.0f)).Length() > 0.9f) {
                if (choose_mat < 0.8f) {
                    // Lambertian
                    list[i++] = new Sphere(center, 0.2f, 
                        new Lambertian(Vec3(FastRandom::random() * FastRandom::random(),
                                          FastRandom::random() * FastRandom::random(),
                                          FastRandom::random() * FastRandom::random())));
                } else if (choose_mat < 0.95f) {
                    // Metal
                    list[i++] = new Sphere(center, 0.2f,
                        new Metal(Vec3(0.5f * (1.0f + FastRandom::random()),
                                     0.5f * (1.0f + FastRandom::random()),
                                     0.5f * (1.0f + FastRandom::random())),
                                0.5f * FastRandom::random()));
                } else {
                    // Glass
                    list[i++] = new Sphere(center, 0.2f, new Dielectric(1.5f));
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

// Progressive tile-based renderer with display updates
void render_tile_progressive(const RenderTile &tile, std::vector<Vec3> &pixels, 
                           int nx, int ny, const Camera &camera, 
                           Hittable *world, RenderStats &tile_stats,
                           ProgressiveRenderer* renderer) {
    PerformanceTimer tile_timer;
    tile_timer.begin();

    for (int j = tile.y_start; j < tile.y_start + tile.height; ++j) {
        for (int i = tile.x_start; i < tile.x_start + tile.width; ++i) {
            // Check if rendering should be paused or stopped
            while (renderer->IsPaused() && !renderer->ShouldStop()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            
            if (renderer->ShouldStop()) {
                return;
            }

            Vec3 col(0.0f, 0.0f, 0.0f);
            
            // Use adaptive sampling for better performance/quality trade-off
            int samples = tile.samples_per_pixel;
            for (int s = 0; s < samples; ++s) {
                float u = float(i + FastRandom::random()) / float(nx);
                float v = float(j + FastRandom::random()) / float(ny);

                Ray ray = camera.GetRay(u, v);
                col += ColorOptimized(ray, world, 0, 8); // Reduced max depth
                tile_stats.rays_traced++;
            }

            col /= float(samples);
            col = Vec3(sqrtf(col[0]), sqrtf(col[1]), sqrtf(col[2])); // Gamma correction

            pixels[j * nx + i] = col;
            
            // Update display immediately for this pixel
            renderer->GetDisplay()->UpdatePixel(i, j, col);
        }
        
        // Update progress every row
        int completed_pixels = (j - tile.y_start + 1) * tile.width;
        int total_tile_pixels = tile.width * tile.height;
        // This is a rough approximation - in real implementation you'd track global progress
    }

    tile_stats.total_time_ms = tile_timer.elapsed_ms();
}

int main(int argc, char const *argv[]) {
    // Optimized render parameters for better performance
    int nx = 800;  // Reduced from 2560
    int ny = 600;  // Reduced from 1440  
    int ns = 4;    // Reduced from 10 samples
    
    // Window size (can be larger than render resolution)
    int window_width = 1200;
    int window_height = 900;

    std::cout << "=== Interactive Raytracer ===\n";
    std::cout << "Render Resolution: " << nx << "x" << ny << "\n";
    std::cout << "Window Size: " << window_width << "x" << window_height << "\n";
    std::cout << "Samples per pixel: " << ns << "\n";

#ifdef USE_OPENMP
    int num_threads = std::min(8, omp_get_max_threads()); // Limit threads for better responsiveness
    omp_set_num_threads(num_threads);
    std::cout << "OpenMP threads: " << num_threads << "\n";
#else
    int num_threads = std::min(8u, std::thread::hardware_concurrency());
    std::cout << "Hardware threads: " << num_threads << "\n";
#endif

    // Initialize progressive renderer
    ProgressiveRenderer renderer(window_width, window_height, nx, ny);
    if (!renderer.Initialize()) {
        std::cerr << "Failed to initialize renderer\n";
        return -1;
    }

    // Performance measurement
    PerformanceTimer total_timer;
    RenderStats final_stats;
    total_timer.begin();

    // Initialize output buffer
    std::vector<Vec3> pixels(nx * ny);

    // Scene setup
    std::unique_ptr<Hittable> world(CreateOptimizedScene());

    // Camera setup
    Vec3 lookFrom(13.0f, 2.0f, 3.0f);
    Vec3 lookAt(0.0f, 0.0f, 0.0f);
    float distToFocus = 10.0f;
    float aperture = 0.0f; // No DOF for better performance

    Camera camera(lookFrom, lookAt, Vec3(0.0f, 1.0f, 0.0f), 
                  20.0f, float(nx) / float(ny), aperture, distToFocus);

    std::cout << "Rendering started. Press 'P' to pause, 'S' to toggle stats, ESC to exit.\n";

    // Create smaller tiles for more responsive updates
    auto tiles = create_tiles(nx, ny, ns, 32); // Smaller tile size
    std::vector<RenderStats> tile_stats(tiles.size());
    
    std::atomic<int> completed_tiles{0};
    std::mutex stats_mutex;

    // Start rendering in separate thread
    std::thread render_thread([&]() {
#ifdef USE_OPENMP
#pragma omp parallel for schedule(dynamic, 1)
        for (size_t t = 0; t < tiles.size(); ++t) {
            if (renderer.ShouldStop()) continue;
            
            render_tile_progressive(tiles[t], pixels, nx, ny, camera, 
                                  world.get(), tile_stats[t], &renderer);
            
            completed_tiles++;
            
            // Update global progress
            renderer.GetDisplay()->SetProgress(completed_tiles.load() * tiles[0].width * tiles[0].height, 
                                            nx * ny);
        }
#else
        ThreadPool pool(num_threads);
        std::vector<std::function<void()>> tasks;
        
        for (size_t t = 0; t < tiles.size(); ++t) {
            tasks.emplace_back([&, t]() {
                if (renderer.ShouldStop()) return;
                
                render_tile_progressive(tiles[t], pixels, nx, ny, camera, 
                                      world.get(), tile_stats[t], &renderer);
                
                completed_tiles++;
                renderer.GetDisplay()->SetProgress(completed_tiles.load() * tiles[0].width * tiles[0].height, 
                                                nx * ny);
            });
        }
        
        pool.enqueue_tasks(tasks);
        pool.wait_all();
#endif
    });

    // Main display loop
    while (!renderer.ShouldClose()) {
        renderer.HandleInput();
        
        float elapsed_time = total_timer.elapsed_ms() / 1000.0f;
        renderer.GetDisplay()->SetRenderTime(elapsed_time);
        
        renderer.GetDisplay()->BeginFrame();
        renderer.GetDisplay()->EndFrame();
        
        // Small delay to prevent excessive CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    }

    // Stop rendering and wait for thread
    renderer.StopRendering();
    if (render_thread.joinable()) {
        render_thread.join();
    }

    // Combine statistics
    for (const auto &stats : tile_stats) {
        final_stats.rays_traced += stats.rays_traced.load();
        final_stats.intersection_tests += stats.intersection_tests.load();
        final_stats.material_evaluations += stats.material_evaluations.load();
    }

    final_stats.total_time_ms = total_timer.elapsed_ms();

    std::cout << "\nRendering complete!\n";
    final_stats.print_stats();

    // Write output file
    std::cout << "Writing output file...\n";
    std::ofstream output_file("output_interactive.ppm");
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
    std::cout << "Output written to output_interactive.ppm\n";

    renderer.Shutdown();
    return 0;
}