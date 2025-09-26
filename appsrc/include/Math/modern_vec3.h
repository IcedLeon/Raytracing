#ifndef MODERN_VEC3_H
#define MODERN_VEC3_H

// Enable GLM SIMD optimizations
#define GLM_FORCE_INLINE
#define GLM_FORCE_SIMD_AVX2
#define GLM_FORCE_ALIGNED_GENTYPES

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>
#include <iostream>
#include <random>

// Modern Vec3 class using GLM for SIMD optimization
class ModernVec3 {
public:
    glm::vec3 data;
    
    // Constructors
    ModernVec3() : data(0.0f, 0.0f, 0.0f) {}
    ModernVec3(float x, float y, float z) : data(x, y, z) {}
    ModernVec3(const glm::vec3& v) : data(v) {}
    
    // Accessors for backward compatibility
    float GetX() const { return data.x; }
    float GetY() const { return data.y; }
    float GetZ() const { return data.z; }
    
    float GetR() const { return data.r; }
    float GetG() const { return data.g; }
    float GetB() const { return data.b; }
    
    // Array access operators
    float operator[](int index) const { return data[index]; }
    float& operator[](int index) { return data[index]; }
    
    // Unary operators
    const ModernVec3& operator+() const { return *this; }
    ModernVec3 operator-() const { return ModernVec3(-data); }
    
    // Assignment operators - SIMD optimized through GLM
    ModernVec3& operator+=(const ModernVec3& rhs) { 
        data += rhs.data; 
        return *this; 
    }
    
    ModernVec3& operator-=(const ModernVec3& rhs) { 
        data -= rhs.data; 
        return *this; 
    }
    
    ModernVec3& operator*=(const ModernVec3& rhs) { 
        data *= rhs.data; 
        return *this; 
    }
    
    ModernVec3& operator/=(const ModernVec3& rhs) { 
        data /= rhs.data; 
        return *this; 
    }
    
    ModernVec3& operator*=(float scalar) { 
        data *= scalar; 
        return *this; 
    }
    
    ModernVec3& operator/=(float scalar) { 
        data /= scalar; 
        return *this; 
    }
    
    // Math functions - SIMD optimized
    float Length() const { 
        return glm::length(data); 
    }
    
    float SquaredLength() const { 
        return glm::length2(data); 
    }
    
    void MakeUnitVector() { 
        data = glm::normalize(data); 
    }
    
    // Conversion to glm::vec3 for seamless integration
    operator glm::vec3() const { return data; }
    glm::vec3& get_glm() { return data; }
    const glm::vec3& get_glm() const { return data; }
};

// Binary operators - SIMD optimized through GLM
inline ModernVec3 operator+(const ModernVec3& lhs, const ModernVec3& rhs) {
    return ModernVec3(lhs.data + rhs.data);
}

inline ModernVec3 operator-(const ModernVec3& lhs, const ModernVec3& rhs) {
    return ModernVec3(lhs.data - rhs.data);
}

inline ModernVec3 operator*(const ModernVec3& lhs, const ModernVec3& rhs) {
    return ModernVec3(lhs.data * rhs.data);
}

inline ModernVec3 operator/(const ModernVec3& lhs, const ModernVec3& rhs) {
    return ModernVec3(lhs.data / rhs.data);
}

inline ModernVec3 operator*(float scalar, const ModernVec3& vec) {
    return ModernVec3(scalar * vec.data);
}

inline ModernVec3 operator*(const ModernVec3& vec, float scalar) {
    return ModernVec3(vec.data * scalar);
}

inline ModernVec3 operator/(const ModernVec3& vec, float scalar) {
    return ModernVec3(vec.data / scalar);
}

// Math functions - SIMD optimized
inline float Dot(const ModernVec3& lhs, const ModernVec3& rhs) {
    return glm::dot(lhs.data, rhs.data);
}

inline ModernVec3 Cross(const ModernVec3& lhs, const ModernVec3& rhs) {
    return ModernVec3(glm::cross(lhs.data, rhs.data));
}

inline ModernVec3 Unit_Vector(const ModernVec3& vec) {
    return ModernVec3(glm::normalize(vec.data));
}

// Stream operators
inline std::istream& operator>>(std::istream& stream, ModernVec3& vec) {
    stream >> vec.data.x >> vec.data.y >> vec.data.z;
    return stream;
}

inline std::ostream& operator<<(std::ostream& stream, const ModernVec3& vec) {
    stream << "X: " << vec.data.x << " Y: " << vec.data.y << " Z: " << vec.data.z;
    return stream;
}

// Fast random number generation for better performance
class FastRandom {
private:
    static thread_local std::mt19937 generator;
    static thread_local std::uniform_real_distribution<float> distribution;
    
public:
    static float random() {
        return distribution(generator);
    }
    
    static void seed(unsigned int s) {
        generator.seed(s);
    }
};

// Thread-local random number generator initialization
thread_local std::mt19937 FastRandom::generator{std::random_device{}()};
thread_local std::uniform_real_distribution<float> FastRandom::distribution{0.0f, 1.0f};

// Optimized random vector generation
inline ModernVec3 RandomInUnitSphere() {
    ModernVec3 p;
    do {
        p = 2.0f * ModernVec3(FastRandom::random(), FastRandom::random(), FastRandom::random()) - ModernVec3(1.0f, 1.0f, 1.0f);
    } while (p.SquaredLength() >= 1.0f);
    return p;
}

// Type alias for backward compatibility
using Vec3Modern = ModernVec3;

#endif // MODERN_VEC3_H