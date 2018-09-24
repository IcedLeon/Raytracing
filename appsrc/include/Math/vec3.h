#ifndef VEC3_H
#define VEC3_H

#include <math.h>
#include <stdlib.h>
#include <iostream>

class Vec3
{
public:
    Vec3();

    Vec3(const float& a_cfE0, const float& a_cfE1, const float& a_cfE2);

    float GetX() const;

    float GetY() const;

    float GetZ() const;

    float GetR() const;

    float GetG() const;

    float GetB() const;

    const Vec3& operator+() const;

    Vec3 operator-() const;

    float operator[](int a_iIndex) const;

    float& operator[](int a_iIndex);

    Vec3& operator+=(const Vec3& a_coRhs);
    Vec3& operator-=(const Vec3& a_coRhs);
    Vec3& operator/=(const Vec3& a_coRhs);
    Vec3& operator*=(const Vec3& a_coRhs);

    Vec3& operator*=(float a_cfDiscriminant);
    Vec3& operator/=(float a_cfDiscriminant);

    float Length() const;

    float SquaredLength() const;

    void MakeUnitVector();

    float m_fAxis[3];

};

inline std::istream& operator >>(std::istream& a_sStreamIn, Vec3& a_oVector)
{
    a_sStreamIn >> a_oVector.m_fAxis[0] >> a_oVector.m_fAxis[1] >> a_oVector.m_fAxis[2];

    return a_sStreamIn;
}

inline std::ostream& operator <<(std::ostream& a_sStreamOut, const Vec3& a_oVector)
{
    a_sStreamOut << "X: " << a_oVector.m_fAxis[0]
                 << "Y: " << a_oVector.m_fAxis[1]
                 << "Z: " << a_oVector.m_fAxis[2];

    return a_sStreamOut;
}

inline Vec3 operator+(const Vec3& a_oLhs, const Vec3& a_oRhs)
{
    return Vec3(a_oLhs.m_fAxis[0] + a_oRhs.m_fAxis[0],
            a_oLhs.m_fAxis[1] + a_oRhs.m_fAxis[1],
            a_oLhs.m_fAxis[2] + a_oRhs.m_fAxis[2]);
}

inline Vec3 operator-(const Vec3& a_oLhs, const Vec3& a_oRhs)
{
    return Vec3(a_oLhs.m_fAxis[0] - a_oRhs.m_fAxis[0],
            a_oLhs.m_fAxis[1] - a_oRhs.m_fAxis[1],
            a_oLhs.m_fAxis[2] - a_oRhs.m_fAxis[2]);
}

inline Vec3 operator*(const Vec3& a_oLhs, const Vec3& a_oRhs)
{
    return Vec3(a_oLhs.m_fAxis[0] * a_oRhs.m_fAxis[0],
            a_oLhs.m_fAxis[1] * a_oRhs.m_fAxis[1],
            a_oLhs.m_fAxis[2] * a_oRhs.m_fAxis[2]);
}

inline Vec3 operator/(const Vec3& a_oLhs, const Vec3& a_oRhs)
{
    return Vec3(a_oLhs.m_fAxis[0] / a_oRhs.m_fAxis[0],
            a_oLhs.m_fAxis[1] / a_oRhs.m_fAxis[1],
            a_oLhs.m_fAxis[2] / a_oRhs.m_fAxis[2]);
}

inline Vec3 operator*(float a_fDisc, const Vec3& a_oVector)
{
    return Vec3(a_fDisc * a_oVector.m_fAxis[0],
            a_fDisc * a_oVector.m_fAxis[1],
            a_fDisc * a_oVector.m_fAxis[2]);
}

inline Vec3 operator*(const Vec3& a_oVector, float a_fDisc)
{
    return Vec3(a_oVector.m_fAxis[0] * a_fDisc,
            a_oVector.m_fAxis[1] * a_fDisc,
            a_oVector.m_fAxis[2] * a_fDisc);
}

inline Vec3 operator/(const Vec3& a_oVector, float a_fDisc)
{
    return Vec3(a_oVector.m_fAxis[0] / a_fDisc,
            a_oVector.m_fAxis[1] / a_fDisc,
            a_oVector.m_fAxis[2] / a_fDisc);
}

inline float Dot(const Vec3& a_oLhs, const Vec3& a_oRhs)
{
    return (a_oLhs.m_fAxis[0] * a_oRhs.m_fAxis[0]) +
           (a_oLhs.m_fAxis[1] * a_oRhs.m_fAxis[1]) +
           (a_oLhs.m_fAxis[2] * a_oRhs.m_fAxis[2]);
}

inline Vec3 Cross(const Vec3& a_oLhs, const Vec3& a_oRhs)
{
    return Vec3(a_oLhs.m_fAxis[1] * a_oRhs.m_fAxis[2] - a_oLhs.m_fAxis[2] * a_oRhs.m_fAxis[1],
            (-(a_oLhs.m_fAxis[0] * a_oRhs.m_fAxis[2] - a_oLhs.m_fAxis[2] * a_oRhs.m_fAxis[0])),
            a_oLhs.m_fAxis[0] * a_oRhs.m_fAxis[1] - a_oLhs.m_fAxis[1] * a_oRhs.m_fAxis[0]);
}

inline Vec3 Unit_Vector(Vec3 a_oVector)
{
    return a_oVector / a_oVector.Length();
}

#endif // VEC3_H
