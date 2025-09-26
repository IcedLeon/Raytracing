#ifndef PERFORMANCE_H
#define PERFORMANCE_H

#include <chrono>
#include <thread>
#include <atomic>
#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <cstdio>

// High-resolution performance timer
class PerformanceTimer {
    std::chrono::high_resolution_clock::time_point start;
    
public:
    void begin() { 
        start = std::chrono::high_resolution_clock::now(); 
    }
    
    double elapsed_ms() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start).count();
    }
    
    double elapsed_seconds() const {
        return elapsed_ms() / 1000.0;
    }
};

// Performance counters for raytracing statistics
struct RenderStats {
    std::atomic<size_t> rays_traced{0};
    std::atomic<size_t> intersection_tests{0};
    std::atomic<size_t> material_evaluations{0};
    double total_time_ms = 0;
    double intersection_time_ms = 0;
    double shading_time_ms = 0;
    
    void reset() {
        rays_traced = 0;
        intersection_tests = 0;
        material_evaluations = 0;
        total_time_ms = 0;
        intersection_time_ms = 0;
        shading_time_ms = 0;
    }
    
    void print_stats() const {
        printf("=== Render Statistics ===\n");
        printf("Total time: %.2f ms (%.2f seconds)\n", total_time_ms, total_time_ms / 1000.0);
        printf("Rays traced: %zu\n", rays_traced.load());
        printf("Intersection tests: %zu\n", intersection_tests.load());
        printf("Material evaluations: %zu\n", material_evaluations.load());
        printf("Rays per second: %.0f\n", rays_traced.load() / (total_time_ms / 1000.0));
        printf("========================\n");
    }
};

// Thread pool for parallel rendering
class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::vector<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop = false;
    size_t next_task = 0;
    
public:
    ThreadPool(size_t threads = std::thread::hardware_concurrency()) {
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        condition.wait(lock, [this] { return stop || next_task < tasks.size(); });
                        
                        if (stop && next_task >= tasks.size()) return;
                        
                        task = tasks[next_task++];
                    }
                    
                    task();
                }
            });
        }
    }
    
    template<class F>
    void enqueue_tasks(const std::vector<F>& task_list) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            tasks.clear();
            next_task = 0;
            
            for (const auto& task : task_list) {
                tasks.emplace_back(task);
            }
        }
        condition.notify_all();
    }
    
    void wait_all() {
        while (true) {
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                if (next_task >= tasks.size()) break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    
    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread &worker: workers) {
            worker.join();
        }
    }
    
    size_t thread_count() const { return workers.size(); }
};

// Render tile for multi-threaded rendering
struct RenderTile {
    int x_start, y_start;
    int width, height;
    int samples_per_pixel;
    
    RenderTile(int x, int y, int w, int h, int samples) 
        : x_start(x), y_start(y), width(w), height(h), samples_per_pixel(samples) {}
};

// Create tiles for efficient parallel rendering
inline std::vector<RenderTile> create_tiles(int image_width, int image_height, int samples, int tile_size = 64) {
    std::vector<RenderTile> tiles;
    
    for (int y = 0; y < image_height; y += tile_size) {
        for (int x = 0; x < image_width; x += tile_size) {
            int tile_width = std::min(tile_size, image_width - x);
            int tile_height = std::min(tile_size, image_height - y);
            tiles.emplace_back(x, y, tile_width, tile_height, samples);
        }
    }
    
    return tiles;
}

#endif // PERFORMANCE_H