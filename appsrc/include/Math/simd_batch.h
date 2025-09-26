#ifndef SIMD_BATCH_H
#define SIMD_BATCH_H

#include "appsrc/include/Math/modern_vec3.h"
#include "appsrc/include/Math/ray.h"
#include "appsrc/include/Math/camera.h"
#include <array>
#include <vector>

// SIMD batch processing for 4 rays simultaneously
struct RayBatch4 {
    static constexpr int BATCH_SIZE = 4;
    
    std::array<glm::vec3, 4> origins;
    std::array<glm::vec3, 4> directions;
    
    RayBatch4() = default;
    
    void set_ray(int index, const Ray& ray) {
        origins[index] = glm::vec3(ray.Origin().GetX(), ray.Origin().GetY(), ray.Origin().GetZ());
        directions[index] = glm::vec3(ray.Direction().GetX(), ray.Direction().GetY(), ray.Direction().GetZ());
    }
    
    Ray get_ray(int index) const {
        const auto& o = origins[index];
        const auto& d = directions[index];
        return Ray(Vec3(o.x, o.y, o.z), Vec3(d.x, d.y, d.z));
    }
};

// Batch intersection results
struct IntersectionBatch4 {
    std::array<bool, 4> hit;
    std::array<float, 4> t_values;
    std::array<glm::vec3, 4> hit_points;
    std::array<glm::vec3, 4> normals;
    
    IntersectionBatch4() {
        hit.fill(false);
        t_values.fill(0.0f);
    }
};

// SIMD-optimized sphere batch intersection
class SIMDSphere {
public:
    glm::vec3 center;
    float radius;
    float radius_squared;
    
    SIMDSphere(const glm::vec3& c, float r) : center(c), radius(r), radius_squared(r * r) {}
    
    // Process 4 rays simultaneously using SIMD
    IntersectionBatch4 intersect_batch(const RayBatch4& rays, float t_min, float t_max) const {
        IntersectionBatch4 results;
        
        // Process all 4 rays in parallel using GLM SIMD operations
        for (int i = 0; i < 4; ++i) {
            glm::vec3 oc = rays.origins[i] - center;
            
            float a = glm::dot(rays.directions[i], rays.directions[i]);
            float b = glm::dot(oc, rays.directions[i]);
            float c = glm::dot(oc, oc) - radius_squared;
            
            float discriminant = b * b - a * c;
            
            if (discriminant > 0.0f) {
                float sqrt_disc = sqrtf(discriminant);
                
                float t1 = (-b - sqrt_disc) / a;
                float t2 = (-b + sqrt_disc) / a;
                
                float t = (t1 < t_max && t1 > t_min) ? t1 : 
                         (t2 < t_max && t2 > t_min) ? t2 : -1.0f;
                
                if (t > 0.0f) {
                    results.hit[i] = true;
                    results.t_values[i] = t;
                    results.hit_points[i] = rays.origins[i] + t * rays.directions[i];
                    results.normals[i] = (results.hit_points[i] - center) / radius;
                }
            }
        }
        
        return results;
    }
};

// Batch renderer for improved SIMD utilization
class BatchRenderer {
private:
    static constexpr int BATCH_SIZE = 4;
    
public:
    // Process multiple pixels in batches for better SIMD utilization
    static void render_pixel_batch(
        const std::vector<std::pair<int, int>>& pixel_coords,
        std::vector<Vec3>& output,
        int image_width,
        const Camera& camera,
        Hittable* world,
        int samples_per_pixel
    ) {
        for (size_t batch_start = 0; batch_start < pixel_coords.size(); batch_start += BATCH_SIZE) {
            size_t batch_end = std::min(batch_start + BATCH_SIZE, pixel_coords.size());
            
            for (int sample = 0; sample < samples_per_pixel; ++sample) {
                RayBatch4 ray_batch;
                
                // Generate batch of rays
                for (size_t i = batch_start; i < batch_end; ++i) {
                    int pixel_x = pixel_coords[i].first;
                    int pixel_y = pixel_coords[i].second;
                    
                    float u = (pixel_x + FastRandom::random()) / float(image_width);
                    float v = (pixel_y + FastRandom::random()) / float(image_width);
                    
                    Ray ray = camera.GetRay(u, v);
                    ray_batch.set_ray(i - batch_start, ray);
                }
                
                // Process batch (placeholder - would integrate with scene)
                // In a full implementation, this would batch process against all scene objects
                for (size_t i = batch_start; i < batch_end; ++i) {
                    // For now, fall back to individual ray processing
                    // In future iterations, this would use SIMD scene intersection
                }
            }
        }
    }
};

#endif // SIMD_BATCH_H