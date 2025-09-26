#ifndef HITTABLE_H
#define HITTABLE_H

#include "appsrc/include/Math/ray.h"

class Material;

struct HitRecord
{
    float m_fT;
    Vec3 m_oPoint;
    Vec3 m_oNormal;
    Material *m_oMaterial;
};

class Hittable
{
public:
    virtual ~Hittable() = default;
    virtual bool Hit(const Ray &a_oRay, float a_fTMin, float a_fTMax, HitRecord &a_oRecord) const = 0;
};

#endif // HITTABLE_H
