#ifndef SPHERE_H
#define SPHERE_H

#include "appsrc/include/Math/hittable.h"

class Sphere : public Hittable
{
public:
    Sphere();
    Sphere(Vec3 a_oCenter, float a_fRadius, Material* a_oMaterial);

    virtual bool Hit(const Ray& a_oRay, float a_fTMin, float a_fTMax, HitRecord& a_oRecord) const;

    Vec3 m_oCenter;
    float m_fRadius;

    Material* m_oMaterial;
};
#endif // SPHERE_H
