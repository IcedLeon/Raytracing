#include "appsrc/include/Math/sphere.h"

Sphere::Sphere() : m_oCenter(Vec3()),
                   m_fRadius(NULL)
{
}

Sphere::Sphere(Vec3 a_oCenter, float a_fRadius, Material* a_oMaterial)
    : m_oCenter(a_oCenter),
      m_fRadius(a_fRadius),
      m_oMaterial(a_oMaterial)
{
}

bool Sphere::Hit(const Ray &a_oRay, float a_fTMin, float a_fTMax, HitRecord &a_oRecord) const
{
    Vec3 _oc = a_oRay.Origin() - this->m_oCenter;

    float _a = Dot(a_oRay.Direction(), a_oRay.Direction());

    float _b = Dot(_oc, a_oRay.Direction());

    float _c = Dot(_oc, _oc) - m_fRadius * m_fRadius;

    float _desc = _b * _b - _a * _c;

    if (_desc > 0.0f)
    {
        float _temp = (-_b - sqrtf(_desc)) / _a;

        if (_temp < a_fTMax && _temp > a_fTMin)
        {
            a_oRecord.m_fT = _temp;
            a_oRecord.m_oPoint = a_oRay.PointAtParamenter(a_oRecord.m_fT);
            a_oRecord.m_oNormal = (a_oRecord.m_oPoint - m_oCenter) / m_fRadius;
            a_oRecord.m_oMaterial = m_oMaterial;
            return true;
        }

        _temp = (-_b + sqrtf(_desc)) / _a;

        if (_temp < a_fTMax && _temp > a_fTMin)
        {
            a_oRecord.m_fT = _temp;
            a_oRecord.m_oPoint = a_oRay.PointAtParamenter(a_oRecord.m_fT);
            a_oRecord.m_oNormal = (a_oRecord.m_oPoint - m_oCenter) / m_fRadius;
            a_oRecord.m_oMaterial = m_oMaterial;
            return true;
        }
    }
    return false;
}
