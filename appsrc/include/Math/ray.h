#ifndef RAY_H
#define RAY_H

#include "appsrc/include/Math/vec3.h"

namespace rt
{

    class Ray
    {
    public:
        Ray();
        Ray(const Vec3 &a_oPointA, const Vec3 &a_oPointB);

        Vec3 Origin() const;

        Vec3 Direction() const;

        Vec3 PointAtParamenter(float a_fT) const;

        Vec3 m_oOrigin;

        Vec3 m_oDirection;
    };

} // namespace rt

#endif // RAY_H
