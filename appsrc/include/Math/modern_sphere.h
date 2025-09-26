#ifndef MODERN_SPHERE_H
#define MODERN_SPHERE_H

#include "appsrc/include/Math/hittable.h"
#include "appsrc/include/Math/modern_vec3.h"

// Modern sphere class using GLM for SIMD-optimized intersection
class ModernSphere : public Hittable
{
public:
    ModernSphere() : m_center(0.0f, 0.0f, 0.0f), m_radius(0.0f), m_radius_squared(0.0f), m_material(nullptr) {}
    
    ModernSphere(const ModernVec3& center, float radius, Material* material)
        : m_center(center), m_radius(radius), m_radius_squared(radius * radius), m_material(material) {}
    
    // Backward compatibility constructor
    ModernSphere(const Vec3& center, float radius, Material* material)
        : m_center(center.GetX(), center.GetY(), center.GetZ()), m_radius(radius), 
          m_radius_squared(radius * radius), m_material(material) {}

    virtual bool Hit(const Ray& ray, float t_min, float t_max, HitRecord& record) const override;

    // SIMD-optimized intersection using GLM
    bool Hit_SIMD(const Ray& ray, float t_min, float t_max, HitRecord& record) const;

    // Getters for backward compatibility
    Vec3 GetCenter() const { return Vec3(m_center.GetX(), m_center.GetY(), m_center.GetZ()); }
    float GetRadius() const { return m_radius; }

    ModernVec3 m_center;
    float m_radius;
    float m_radius_squared;  // Cached for performance
    Material* m_material;
};

// SIMD-optimized sphere intersection implementation
inline bool ModernSphere::Hit_SIMD(const Ray& ray, float t_min, float t_max, HitRecord& record) const
{
    // Convert Ray to GLM for SIMD operations
    glm::vec3 origin(ray.Origin().GetX(), ray.Origin().GetY(), ray.Origin().GetZ());
    glm::vec3 direction(ray.Direction().GetX(), ray.Direction().GetY(), ray.Direction().GetZ());
    
    // Vector from ray origin to sphere center
    glm::vec3 oc = origin - m_center.data;
    
    // Quadratic equation coefficients - SIMD optimized
    float a = glm::dot(direction, direction);
    float b = glm::dot(oc, direction);
    float c = glm::dot(oc, oc) - m_radius_squared;
    
    // Discriminant
    float discriminant = b * b - a * c;
    
    if (discriminant > 0.0f) {
        float sqrt_disc = sqrtf(discriminant);
        
        // Try nearest intersection first
        float t = (-b - sqrt_disc) / a;
        if (t < t_max && t > t_min) {
            glm::vec3 hit_point = origin + t * direction;
            glm::vec3 normal = (hit_point - m_center.data) / m_radius;
            
            record.m_fT = t;
            record.m_oPoint = Vec3(hit_point.x, hit_point.y, hit_point.z);
            record.m_oNormal = Vec3(normal.x, normal.y, normal.z);
            record.m_oMaterial = m_material;
            return true;
        }
        
        // Try further intersection
        t = (-b + sqrt_disc) / a;
        if (t < t_max && t > t_min) {
            glm::vec3 hit_point = origin + t * direction;
            glm::vec3 normal = (hit_point - m_center.data) / m_radius;
            
            record.m_fT = t;
            record.m_oPoint = Vec3(hit_point.x, hit_point.y, hit_point.z);
            record.m_oNormal = Vec3(normal.x, normal.y, normal.z);
            record.m_oMaterial = m_material;
            return true;
        }
    }
    
    return false;
}

// Fallback to standard intersection for compatibility
inline bool ModernSphere::Hit(const Ray& ray, float t_min, float t_max, HitRecord& record) const
{
    return Hit_SIMD(ray, t_min, t_max, record);
}

#endif // MODERN_SPHERE_H