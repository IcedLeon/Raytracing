#include <iostream>
#include <fstream>
#include <float.h>
#include <vector>
#include <memory>
#include <execution>
#include <algorithm>
#include <ranges>

#ifdef USE_OPENMP
#include <omp.h>
#endif

#include "appsrc/include/Math/sphere.h"
#include "appsrc/include/Math/hittablelist.h"
#include "appsrc/include/Math/camera.h"
#include "appsrc/include/Math/material.h"
#include "appsrc/include/Math/performance.h"

// Thread-local statistics
thread_local RenderStats thread_stats;

Vec3 Color(const Ray &a_oRay, Hittable *a_oWorld, int a_iDepth, RenderStats *stats = nullptr)
{
    if (stats)
    {
        stats->rays_traced++;
    }

    HitRecord _record;

    if (a_oWorld->Hit(a_oRay, 0.001f, FLT_MAX, _record))
    {
        if (stats)
        {
            stats->intersection_tests++;
        }

        Ray _scatter;
        Vec3 _attenuation;

        if (a_iDepth < 50 && _record.m_oMaterial->Scatter(a_oRay, _record, _attenuation, _scatter))
        {
            if (stats)
            {
                stats->material_evaluations++;
            }
            return _attenuation * Color(_scatter, a_oWorld, a_iDepth + 1, stats);
        }
        else
        {
            return Vec3(0.0f, 0.0f, 0.0f);
        }
    }
    else
    {
        Vec3 _unitDir = Unit_Vector(a_oRay.Direction());
        float _t = 0.5f * (_unitDir.GetY() + 1.0f);
        return (1.0f - _t) * Vec3(1.0f, 1.0f, 1.0f) + _t * Vec3(0.5f, 0.7f, 1.0f);
    }
}

Hittable *RandomScene()
{
    int n = 500;
    Hittable **_list = new Hittable *[n + 1];

    _list[0] = new Sphere(Vec3(0.0f, -1000.0f, 0.0f), 1000.0f, new Lambertian(Vec3(0.5f, 0.5f, 0.5f)));

    int i = 1;

    for (int a = -11; a < 11; ++a)
    {
        for (int b = -11; b < 11; ++b)
        {
            float _chooseMat = (std::rand() / (RAND_MAX + 1.0));
            Vec3 _center(a + 0.9f * (std::rand() / (RAND_MAX + 1.0)), 0.2f, b + 0.9f * (std::rand() / (RAND_MAX + 1.0)));

            if ((_center - Vec3(4.0f, 0.2f, 0.0f)).Length() > 0.9f)
            {
                if (_chooseMat < 0.8f)
                {
                    _list[i++] = new Sphere(_center, 0.2f, new Lambertian(Vec3((std::rand() / (RAND_MAX + 1.0)) * (std::rand() / (RAND_MAX + 1.0)), (std::rand() / (RAND_MAX + 1.0)) * (std::rand() / (RAND_MAX + 1.0)), (std::rand() / (RAND_MAX + 1.0)) * (std::rand() / (RAND_MAX + 1.0)))));
                }
                else if (_chooseMat < 0.95f)
                {
                    _list[i++] = new Sphere(_center, 0.2f,
                                            new Metal(Vec3(0.5f * (1.0f + (std::rand() / (RAND_MAX + 1.0))), 0.5f * (1.0f + (std::rand() / (RAND_MAX + 1.0))), 0.5f * (1.0f + (std::rand() / (RAND_MAX + 1.0)))), 0.5f * (std::rand() / (RAND_MAX + 1.0))));
                }
                else
                {
                    _list[i++] = new Sphere(_center, 0.2f, new Dielectric(1.5f));
                }
            }
        }
    }

    _list[i++] = new Sphere(Vec3(0.0f, 1.0f, 0.0f), 1.0f, new Dielectric(1.5f));
    _list[i++] = new Sphere(Vec3(-4.0f, 1.0f, 0.0f), 1.0f, new Lambertian(Vec3(0.4f, 0.2f, 0.1f)));
    _list[i++] = new Sphere(Vec3(4.0f, 1.0f, 0.0f), 1.0f, new Metal(Vec3(0.7f, 0.6f, 0.5f), 0.0f));

    return new HittableList(_list, i);
}

// Multi-threaded tile renderer
void render_tile(const RenderTile &tile, std::vector<Vec3> &pixels, int nx, int ny,
                 const Camera &camera, Hittable *world, RenderStats &tile_stats)
{

    for (int j = tile.y_start; j < tile.y_start + tile.height; ++j)
    {
        for (int i = tile.x_start; i < tile.x_start + tile.width; ++i)
        {
            Vec3 col(0.0f, 0.0f, 0.0f);

            for (int s = 0; s < tile.samples_per_pixel; ++s)
            {
                float u = float(i + (std::rand() / (RAND_MAX + 1.0))) / float(nx);
                float v = float(j + (std::rand() / (RAND_MAX + 1.0))) / float(ny);

                Ray ray = camera.GetRay(u, v);
                col += Color(ray, world, 0, &tile_stats);
            }

            col /= float(tile.samples_per_pixel);
            col = Vec3(sqrtf(col[0]), sqrtf(col[1]), sqrtf(col[2])); // Gamma correction

            pixels[j * nx + i] = col;
        }
    }
}

int main(int argc, char const *argv[])
{
    // Render parameters 2k
    int nx = 2560;
    int ny = 1440;
    int ns = 10;

    std::cout << "=== Modern Raytracer ===" << std::endl;
    std::cout << "Resolution: " << nx << "x" << ny << std::endl;
    std::cout << "Samples per pixel: " << ns << std::endl;

#ifdef USE_OPENMP
    int num_threads = omp_get_max_threads();
    std::cout << "OpenMP threads: " << num_threads << std::endl;
#else
    int num_threads = std::thread::hardware_concurrency();
    std::cout << "Hardware threads: " << num_threads << std::endl;
#endif

    // Performance measurement
    PerformanceTimer total_timer;
    RenderStats final_stats;
    total_timer.begin();

    // Initialize output buffer
    std::vector<Vec3> pixels(nx * ny);

    // Scene setup
    std::unique_ptr<Hittable> world(RandomScene());

    Vec3 lookFrom(13.0f, 2.0f, 3.0f);
    Vec3 lookAt(0.0f, 0.0f, 0.0f);
    float distToFocus = 10.0f;
    float aperture = 0.1f;

    Camera camera(lookFrom, lookAt, Vec3(0.0f, 1.0f, 0.0f), 20.0f, float(nx) / float(ny), aperture, distToFocus);

    std::cout << "Rendering..." << std::endl;

#ifdef USE_OPENMP
// OpenMP parallel rendering
#pragma omp parallel for schedule(dynamic, 1) collapse(2)
    for (int j = 0; j < ny; ++j)
    {
        for (int i = 0; i < nx; ++i)
        {
            Vec3 col(0.0f, 0.0f, 0.0f);

            for (int s = 0; s < ns; ++s)
            {
                float u = float(i + (std::rand() / (RAND_MAX + 1.0))) / float(nx);
                float v = float(j + (std::rand() / (RAND_MAX + 1.0))) / float(ny);

                Ray ray = camera.GetRay(u, v);
                col += Color(ray, world.get(), 0);
            }

            col /= float(ns);
            col = Vec3(sqrtf(col[0]), sqrtf(col[1]), sqrtf(col[2]));

            pixels[j * nx + i] = col;
        }
    }
#else
    // Thread pool based rendering
    auto tiles = create_tiles(nx, ny, ns, 64);
    ThreadPool pool(num_threads);
    std::vector<RenderStats> tile_stats(tiles.size());

    std::vector<std::function<void()>> tasks;
    for (size_t t = 0; t < tiles.size(); ++t)
    {
        tasks.emplace_back([&, t]()
                           { render_tile(tiles[t], pixels, nx, ny, camera, world.get(), tile_stats[t]); });
    }

    pool.enqueue_tasks(tasks);
    pool.wait_all();

    // Combine statistics
    for (const auto &stats : tile_stats)
    {
        final_stats.rays_traced += stats.rays_traced.load();
        final_stats.intersection_tests += stats.intersection_tests.load();
        final_stats.material_evaluations += stats.material_evaluations.load();
    }
#endif

    final_stats.total_time_ms = total_timer.elapsed_ms();

    std::cout << "Rendering complete!" << std::endl;
    final_stats.print_stats();

    // Write output
    std::cout << "Writing output file..." << std::endl;
    std::ofstream output_file("output.ppm");
    output_file << "P3\n"
                << nx << " " << ny << "\n255\n";

    for (int j = ny - 1; j >= 0; --j)
    {
        for (int i = 0; i < nx; ++i)
        {
            const Vec3 &col = pixels[j * nx + i];
            int ir = int(255.99 * col[0]);
            int ig = int(255.99 * col[1]);
            int ib = int(255.99 * col[2]);

            output_file << ir << " " << ig << " " << ib << "\n";
        }
    }

    output_file.close();
    std::cout << "Output written to output.ppm" << std::endl;

    return 0;
}