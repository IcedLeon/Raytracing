#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <algorithm>
#include <mutex>

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

// SIMD-optimized color function using GLM
Vec3 ColorSIMD(const Ray& ray, Hittable* world, int depth, RenderStats* stats = nullptr)
{
    if (stats) {
        stats->rays_traced++;
    }
    
    HitRecord record;

    if (world->Hit(ray, 0.001f, FLT_MAX, record))
    {
        if (stats) {
            stats->intersection_tests++;
        }
        
        Ray scattered;
        Vec3 attenuation;

        if (depth < 50 && record.m_oMaterial->Scatter(ray, record, attenuation, scattered))
        {
            if (stats) {
                stats->material_evaluations++;
            }
            return attenuation * ColorSIMD(scattered, world, depth + 1, stats);
        }
        else
        {
            return Vec3(0.0f, 0.0f, 0.0f);
        }
    }
    else
    {
        // SIMD-optimized sky gradient using GLM
        glm::vec3 dir = glm::normalize(glm::vec3(
            ray.Direction().GetX(), 
            ray.Direction().GetY(), 
            ray.Direction().GetZ()
        ));
        
        float t = 0.5f * (dir.y + 1.0f);
        glm::vec3 sky_color = glm::mix(
            glm::vec3(1.0f, 1.0f, 1.0f),      // White
            glm::vec3(0.5f, 0.7f, 1.0f),      // Light blue
            t
        );
        
        return Vec3(sky_color.x, sky_color.y, sky_color.z);
    }
}

// Create scene using mix of modern and legacy spheres for compatibility
Hittable* CreateModernScene()
{
    int n = 500;
    Hittable** list = new Hittable*[n + 1];

    // Ground sphere
    list[0] = new Sphere(Vec3(0.0f, -1000.0f, 0.0f), 1000.0f, new Lambertian(Vec3(0.5f, 0.5f, 0.5f)));

    int i = 1;

    // Generate random spheres with modern spheres where possible
    for (int a = -11; a < 11; ++a)
    {
        for (int b = -11; b < 11; ++b)
        {
            float choose_mat = FastRandom::random();
            ModernVec3 center(a + 0.9f * FastRandom::random(), 0.2f, b + 0.9f * FastRandom::random());

            if ((center - ModernVec3(4.0f, 0.2f, 0.0f)).Length() > 0.9f)
            {
                if (choose_mat < 0.8f)
                {
                    // Lambertian - use modern sphere
                    Vec3 albedo(FastRandom::random() * FastRandom::random(), 
                               FastRandom::random() * FastRandom::random(), 
                               FastRandom::random() * FastRandom::random());
                    list[i++] = new ModernSphere(center, 0.2f, new Lambertian(albedo));
                }
                else if (choose_mat < 0.95f)
                {
                    // Metal - use modern sphere
                    Vec3 albedo(0.5f * (1.0f + FastRandom::random()), 
                               0.5f * (1.0f + FastRandom::random()), 
                               0.5f * (1.0f + FastRandom::random()));
                    list[i++] = new ModernSphere(center, 0.2f, new Metal(albedo, 0.5f * FastRandom::random()));
                }
                else
                {
                    // Dielectric - use modern sphere
                    list[i++] = new ModernSphere(center, 0.2f, new Dielectric(1.5f));
                }
            }
        }
    }

    // Large spheres
    list[i++] = new ModernSphere(ModernVec3(0.0f, 1.0f, 0.0f), 1.0f, new Dielectric(1.5f));
    list[i++] = new ModernSphere(ModernVec3(-4.0f, 1.0f, 0.0f), 1.0f, new Lambertian(Vec3(0.4f, 0.2f, 0.1f)));
    list[i++] = new ModernSphere(ModernVec3(4.0f, 1.0f, 0.0f), 1.0f, new Metal(Vec3(0.7f, 0.6f, 0.5f), 0.0f));

    return new HittableList(list, i);
}

// SIMD-optimized tile renderer
void render_tile_simd(const RenderTile& tile, std::vector<Vec3>& pixels, int nx, int ny, 
                      const Camera& camera, Hittable* world, RenderStats& tile_stats) 
{
    for (int j = tile.y_start; j < tile.y_start + tile.height; ++j) {
        for (int i = tile.x_start; i < tile.x_start + tile.width; ++i) {
            
            // Accumulate color samples using SIMD operations where possible
            glm::vec3 color_accum(0.0f);

            for (int s = 0; s < tile.samples_per_pixel; ++s) {
                float u = float(i + FastRandom::random()) / float(nx);
                float v = float(j + FastRandom::random()) / float(ny);

                Ray ray = camera.GetRay(u, v);
                Vec3 sample_color = ColorSIMD(ray, world, 0, &tile_stats);
                
                // Accumulate using GLM SIMD
                color_accum += glm::vec3(sample_color.GetX(), sample_color.GetY(), sample_color.GetZ());
            }
            
            // Average and gamma correct using SIMD
            color_accum /= float(tile.samples_per_pixel);
            color_accum = glm::sqrt(color_accum);  // SIMD gamma correction

            pixels[j * nx + i] = Vec3(color_accum.x, color_accum.y, color_accum.z);
        }
    }
}

int main(int argc, char const *argv[])
{
    // Render parameters
    int nx = 400;
    int ny = 300;
    int ns = 5;
    
    std::cout << "=== SIMD-Optimized Raytracer ===" << std::endl;
    std::cout << "Resolution: " << nx << "x" << ny << std::endl;
    std::cout << "Samples per pixel: " << ns << std::endl;
    
#ifdef USE_OPENMP
    int num_threads = omp_get_max_threads();
    std::cout << "OpenMP threads: " << num_threads << std::endl;
#else
    int num_threads = std::thread::hardware_concurrency();
    std::cout << "Hardware threads: " << num_threads << std::endl;
#endif

    std::cout << "SIMD: GLM with AVX2 optimizations" << std::endl;

    // Performance measurement
    PerformanceTimer total_timer;
    RenderStats final_stats;
    total_timer.begin();

    // Initialize output buffer
    std::vector<Vec3> pixels(nx * ny);

    // Scene setup using modern components
    std::unique_ptr<Hittable> world(CreateModernScene());

    Vec3 lookFrom(13.0f, 2.0f, 3.0f);
    Vec3 lookAt(0.0f, 0.0f, 0.0f);
    float distToFocus = 10.0f;
    float aperture = 0.1f;

    Camera camera(lookFrom, lookAt, Vec3(0.0f, 1.0f, 0.0f), 20.0f, float(nx) / float(ny), aperture, distToFocus);

    std::cout << "Rendering with SIMD optimizations..." << std::endl;

#ifdef USE_OPENMP
    // OpenMP parallel rendering with SIMD
    #pragma omp parallel for schedule(dynamic, 1) collapse(2)
    for (int j = 0; j < ny; ++j) {
        for (int i = 0; i < nx; ++i) {
            thread_local RenderStats thread_stats;
            
            glm::vec3 color_accum(0.0f);

            for (int s = 0; s < ns; ++s) {
                float u = float(i + FastRandom::random()) / float(nx);
                float v = float(j + FastRandom::random()) / float(ny);

                Ray ray = camera.GetRay(u, v);
                Vec3 sample_color = ColorSIMD(ray, world.get(), 0); // Remove stats collection
                
                color_accum += glm::vec3(sample_color.GetX(), sample_color.GetY(), sample_color.GetZ());
            }

            color_accum /= float(ns);
            color_accum = glm::sqrt(color_accum);  // SIMD gamma correction

            pixels[j * nx + i] = Vec3(color_accum.x, color_accum.y, color_accum.z);
            
            // Accumulate thread stats (use atomic operations properly)
            {
                static std::mutex stats_mutex;
                std::lock_guard<std::mutex> lock(stats_mutex);
                final_stats.rays_traced += thread_stats.rays_traced.load();
                final_stats.intersection_tests += thread_stats.intersection_tests.load();
                final_stats.material_evaluations += thread_stats.material_evaluations.load();
            }
        }
    }
#else
    // Thread pool based rendering with SIMD
    auto tiles = create_tiles(nx, ny, ns, 64);
    ThreadPool pool(num_threads);
    std::vector<RenderStats> tile_stats(tiles.size());
    
    std::vector<std::function<void()>> tasks;
    for (size_t t = 0; t < tiles.size(); ++t) {
        tasks.emplace_back([&, t]() {
            render_tile_simd(tiles[t], pixels, nx, ny, camera, world.get(), tile_stats[t]);
        });
    }
    
    pool.enqueue_tasks(tasks);
    pool.wait_all();
    
    // Combine statistics
    for (const auto& stats : tile_stats) {
        final_stats.rays_traced += stats.rays_traced.load();
        final_stats.intersection_tests += stats.intersection_tests.load();
        final_stats.material_evaluations += stats.material_evaluations.load();
    }
#endif

    final_stats.total_time_ms = total_timer.elapsed_ms();

    std::cout << "SIMD rendering complete!" << std::endl;
    final_stats.print_stats();

    // Write output
    std::cout << "Writing output file..." << std::endl;
    std::ofstream output_file("output_simd.ppm");
    output_file << "P3\n" << nx << " " << ny << "\n255\n";

    for (int j = ny - 1; j >= 0; --j) {
        for (int i = 0; i < nx; ++i) {
            const Vec3& col = pixels[j * nx + i];
            int ir = int(255.99 * col[0]);
            int ig = int(255.99 * col[1]);
            int ib = int(255.99 * col[2]);
            
            output_file << ir << " " << ig << " " << ib << "\n";
        }
    }
    
    output_file.close();
    std::cout << "SIMD output written to output_simd.ppm" << std::endl;
    
    return 0;
}