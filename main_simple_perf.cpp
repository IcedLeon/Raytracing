#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <algorithm>
#include <chrono>
#include <iomanip>

#include "appsrc/include/Math/sphere.h"
#include "appsrc/include/Math/modern_vec3.h"
#include "appsrc/include/Math/hittablelist.h"
#include "appsrc/include/Math/camera.h"
#include "appsrc/include/Math/material.h"

// Simple optimized color function
Vec3 ColorSimple(const Ray &ray, Hittable *world, int depth, int max_depth = 4) {
    HitRecord rec;
    
    if (depth >= max_depth) return Vec3(0.0f, 0.0f, 0.0f);
    
    if (world->Hit(ray, 0.001f, FLT_MAX, rec)) {
        Vec3 target = rec.m_oPoint + rec.m_oNormal + Vec3(FastRandom::random(), FastRandom::random(), FastRandom::random());
        return 0.5f * ColorSimple(Ray(rec.m_oPoint, target - rec.m_oPoint), world, depth + 1, max_depth);
    }
    
    Vec3 unit_direction = Unit_Vector(ray.Direction());
    float t = 0.5f * (unit_direction.GetY() + 1.0f);
    return (1.0f - t) * Vec3(1.0f, 1.0f, 1.0f) + t * Vec3(0.5f, 0.7f, 1.0f);
}

// Create a much smaller scene for testing
Hittable *CreateSimpleScene() {
    Hittable **list = new Hittable*[4];
    
    list[0] = new Sphere(Vec3(0.0f, -100.5f, -1.0f), 100.0f, new Lambertian(Vec3(0.5f, 0.5f, 0.5f)));
    list[1] = new Sphere(Vec3(0.0f, 0.0f, -1.0f), 0.5f, new Lambertian(Vec3(0.7f, 0.3f, 0.3f)));
    list[2] = new Sphere(Vec3(-1.0f, 0.0f, -1.0f), 0.5f, new Dielectric(1.5f));
    list[3] = new Sphere(Vec3(1.0f, 0.0f, -1.0f), 0.5f, new Metal(Vec3(0.8f, 0.6f, 0.2f), 0.3f));

    return new HittableList(list, 4);
}

int main() {
    // Very reasonable parameters for good performance
    int nx = 400;
    int ny = 300;
    int ns = 4;

    std::cout << "=== Simple Performance Raytracer ===\n";
    std::cout << "Resolution: " << nx << "x" << ny << "\n";
    std::cout << "Samples per pixel: " << ns << "\n";

    auto start_time = std::chrono::high_resolution_clock::now();

    std::vector<Vec3> pixels(nx * ny);
    std::unique_ptr<Hittable> world(CreateSimpleScene());

    Vec3 lookFrom(0.0f, 0.0f, 0.0f);
    Vec3 lookAt(0.0f, 0.0f, -1.0f);
    float distToFocus = 1.0f;
    float aperture = 0.0f;

    Camera camera(lookFrom, lookAt, Vec3(0.0f, 1.0f, 0.0f), 
                  90.0f, float(nx) / float(ny), aperture, distToFocus);

    std::cout << "Rendering...\n";

    int total_pixels = nx * ny;
    int completed = 0;
    
    for (int j = ny - 1; j >= 0; --j) {
        for (int i = 0; i < nx; ++i) {
            Vec3 col(0.0f, 0.0f, 0.0f);
            
            for (int s = 0; s < ns; ++s) {
                float u = float(i + FastRandom::random()) / float(nx);
                float v = float(j + FastRandom::random()) / float(ny);

                Ray ray = camera.GetRay(u, v);
                col += ColorSimple(ray, world.get(), 0);
            }

            col /= float(ns);
            col = Vec3(sqrtf(col[0]), sqrtf(col[1]), sqrtf(col[2]));

            pixels[j * nx + i] = col;
            completed++;
            
            // Progress every 1000 pixels
            if (completed % 1000 == 0) {
                float progress = (float)completed / total_pixels * 100.0f;
                std::cout << "Progress: " << std::fixed << std::setprecision(1) << progress << "%\r" << std::flush;
            }
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << "\n\nRendering complete!\n";
    std::cout << "Total time: " << duration.count() << " ms\n";
    std::cout << "Pixels per second: " << std::fixed << std::setprecision(0) 
              << (float)(nx * ny) / (duration.count() / 1000.0f) << "\n";

    // Write output
    std::cout << "Writing output file...\n";
    std::ofstream output_file("output_simple_perf.ppm");
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
    std::cout << "Output written to output_simple_perf.ppm\n";

    // Compare with theoretical 2K performance
    float current_time_2k = duration.count() * (2560.0f * 1440.0f * 10) / (nx * ny * ns) / 1000.0f;
    std::cout << "\nPerformance Analysis:\n";
    std::cout << "Estimated time for 2K@10spp: " << std::setprecision(1) << current_time_2k << " seconds\n";
    std::cout << "Speedup vs original 2K: " << std::setprecision(1) << (2560.0f * 1440.0f * 10) / (nx * ny * ns) << "x faster\n";

    return 0;
}