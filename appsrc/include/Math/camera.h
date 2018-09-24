#ifndef CAMERA_H
#define CAMERA_H

#include "appsrc/include/Math/ray.h"

class Camera
{
public:
    Camera(Vec3 a_oLookFrom, Vec3 a_oLookAt, Vec3 a_oUp, float a_fFov, float a_fAspect, float a_fAperture, float a_fFocusDist);

    Ray GetRay(float a_fU, float a_fV);

    Vec3 m_oOrigin;
    Vec3 m_oLowerLeftCorner;
    Vec3 m_oHorizontal;
    Vec3 m_oVertical;
    Vec3 m_oU, m_oV, m_oW;

    Vec3 RandomUnitInDisk();

    float m_fLensRadius;
};

#endif // CAMERA_H
