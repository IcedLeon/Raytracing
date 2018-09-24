#include "appsrc/include/Math/camera.h"

Camera::Camera(Vec3 a_oLookFrom, Vec3 a_oLookAt, Vec3 a_oUp, float a_fFov, float a_fAspect, float a_fAperture, float a_fFocusDist)
{
    m_fLensRadius = a_fAperture / 2;

    float _theta = a_fFov * M_PI / 180;

    float _halfHeight = tan(_theta / 2);

    float _halfWidth = a_fAspect * _halfHeight;

    m_oOrigin = a_oLookFrom;

    m_oW = Unit_Vector(a_oLookFrom - a_oLookAt);

    m_oU = Unit_Vector(Cross(a_oUp, m_oW));

    m_oV = Cross(m_oW, m_oU);

    m_oLowerLeftCorner = m_oOrigin - _halfWidth * a_fFocusDist * m_oU - _halfHeight * a_fFocusDist * m_oV - a_fFocusDist * m_oW;

    m_oHorizontal = 2 * _halfWidth * a_fFocusDist * m_oU;

    m_oVertical = 2 * _halfHeight * a_fFocusDist * m_oV;
}

Vec3 Camera::RandomUnitInDisk()
{
    Vec3 _p;

    do
    {
        _p = 2.0f * Vec3((std::rand() / (RAND_MAX + 1.0)), (std::rand() / (RAND_MAX + 1.0)), 0.0f) - Vec3(1.0f, 1.0f, 0.0f);
    } while (Dot(_p, _p) >= 1.0f);

    return _p;
}

Ray Camera::GetRay(float a_fU, float a_fV)
{
    Vec3 _rd = m_fLensRadius * RandomUnitInDisk();

    Vec3 _offset = m_oU * _rd.GetX() + m_oV * _rd.GetY();

    return Ray(m_oOrigin + _offset, m_oLowerLeftCorner + a_fU * m_oHorizontal + a_fV * m_oVertical - m_oOrigin - _offset);
}
