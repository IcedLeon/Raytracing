#include "appsrc/include/Math/vec3.h"

Vec3::Vec3()
{
}

Vec3::Vec3(const float &a_cfE0, const float &a_cfE1, const float &a_cfE2)
{
    this->m_fAxis[0] = a_cfE0;
    this->m_fAxis[1] = a_cfE1;
    this->m_fAxis[2] = a_cfE2;
}

float Vec3::GetX() const
{
    return this->m_fAxis[0];
}

float Vec3::GetY() const
{
    return this->m_fAxis[1];
}

float Vec3::GetZ() const
{
    return this->m_fAxis[2];
}

float Vec3::GetR() const
{
    return this->m_fAxis[0];
}

float Vec3::GetG() const
{
    return this->m_fAxis[1];
}

float Vec3::GetB() const
{
    return this->m_fAxis[2];
}

const Vec3& Vec3::operator+() const
{
    return  *this;
}

Vec3 Vec3::operator -() const
{
    return Vec3(-this->m_fAxis[0], -this->m_fAxis[1], -this->m_fAxis[2]);
}

float Vec3::operator[](int a_iIndex) const
{
    return this->m_fAxis[a_iIndex];
}

float& Vec3::operator[](int a_iIndex)
{
    return  this->m_fAxis[a_iIndex];
}

Vec3& Vec3::operator+=(const Vec3& a_oRhs)
{
    this->m_fAxis[0] += a_oRhs.m_fAxis[0];
    this->m_fAxis[1] += a_oRhs.m_fAxis[1];
    this->m_fAxis[2] += a_oRhs.m_fAxis[2];

    return *this;
}

Vec3& Vec3::operator-=(const Vec3& a_oRhs)
{
    this->m_fAxis[0] -= a_oRhs.m_fAxis[0];
    this->m_fAxis[1] -= a_oRhs.m_fAxis[1];
    this->m_fAxis[2] -= a_oRhs.m_fAxis[2];

    return *this;
}

Vec3& Vec3::operator/=(const Vec3& a_oRhs)
{
    this->m_fAxis[0] /= a_oRhs.m_fAxis[0];
    this->m_fAxis[1] /= a_oRhs.m_fAxis[1];
    this->m_fAxis[2] /= a_oRhs.m_fAxis[2];

    return *this;
}

Vec3& Vec3::operator*=(const Vec3& a_oRhs)
{
    this->m_fAxis[0] *= a_oRhs.m_fAxis[0];
    this->m_fAxis[1] *= a_oRhs.m_fAxis[1];
    this->m_fAxis[2] *= a_oRhs.m_fAxis[2];

    return *this;
}

Vec3& Vec3::operator*=(float a_cfDiscriminant)
{
    this->m_fAxis[0] *= a_cfDiscriminant;
    this->m_fAxis[1] *= a_cfDiscriminant;
    this->m_fAxis[2] *= a_cfDiscriminant;

    return *this;
}

Vec3& Vec3::operator/=(float a_cfDiscriminant)
{
    float _k = 1.0f / a_cfDiscriminant;

    this->m_fAxis[0] *= _k;
    this->m_fAxis[1] *= _k;
    this->m_fAxis[2] *= _k;

    return *this;
}

void Vec3::MakeUnitVector()
{
    float _k = 1.0f / sqrtf((this->m_fAxis[0] * this->m_fAxis[0]) + (this->m_fAxis[1] * this->m_fAxis[1]) + (this->m_fAxis[2] * this->m_fAxis[2]));

    this->m_fAxis[0] *= _k;
    this->m_fAxis[1] *= _k;
    this->m_fAxis[2] *= _k;
}

float Vec3::Length() const
{
    return sqrtf(this->m_fAxis[0] * this->m_fAxis[0] + this->m_fAxis[1] * this->m_fAxis[1] + this->m_fAxis[2] * this->m_fAxis[2]);
}

float Vec3::SquaredLength() const
{
    return this->m_fAxis[0] * this->m_fAxis[0] + this->m_fAxis[1] * this->m_fAxis[1] + this->m_fAxis[2] * this->m_fAxis[2];
}
