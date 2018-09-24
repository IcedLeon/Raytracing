#include "appsrc/include/Math/ray.h"

Ray::Ray()
{
}

Ray::Ray(const Vec3 &a_oPointA, const Vec3 &a_oPointB)
    : m_oOrigin(a_oPointA),
      m_oDirection(a_oPointB)
{
}

Vec3 Ray::Origin() const
{
    return this->m_oOrigin;
}

Vec3 Ray::Direction() const
{
    return this->m_oDirection;
}

Vec3 Ray::PointAtParamenter(float a_fT) const
{
    return this->m_oOrigin + a_fT * this->m_oDirection;
}
