#ifndef MATERIAL_H
#define MATERIAL_H

#include "appsrc/include/Math/ray.h"
#include "appsrc/include/Math/hittable.h"


float Schlick(float a_dCosine, float a_dRefIdx)
{
    float _r0 = (1.0f - a_dRefIdx) / (1.0f + a_dRefIdx);

    _r0 = _r0 * _r0;

    return _r0 + (1.0f - _r0) * pow((1.0f - a_dCosine), 5);
}

bool Refract(const Vec3& a_oVecIn, const Vec3& a_oNormal, float a_fNiOverNt, Vec3& a_oRefracted)
{
    Vec3 _uv = Unit_Vector(a_oVecIn);

    float _dt = Dot(_uv, a_oNormal);

    float _disc = 1.0f - a_fNiOverNt * a_fNiOverNt * (1.0f - _dt * _dt);

    if (_disc > 0)
    {
        a_oRefracted = a_fNiOverNt * (_uv - a_oNormal * _dt) - a_oNormal * sqrtf(_disc);
        return true;
    }
    else
    {
        return false;
    }
}

Vec3 Reflect(const Vec3& a_oVecIn, const Vec3& a_oNormal)
{
    return a_oVecIn - 2 * Dot(a_oVecIn, a_oNormal) * a_oNormal;
}

Vec3 RandomInUnitSphere()
{
    Vec3 _p;

    do
    {
        _p = 2.0f * Vec3((std::rand() / (RAND_MAX + 1.0)),
                         (std::rand() / (RAND_MAX + 1.0)),
                         (std::rand() / (RAND_MAX + 1.0))) - Vec3(1.0f, 1.0f, 1.0f);
    } while (_p.SquaredLength() >= 1.0f);

    return _p;
}

class Material
{
public:
    virtual bool Scatter(const Ray& a_oRayIn, const HitRecord& a_oRecord, Vec3& a_oAttenuation, Ray& a_oScatterRay) const = 0;

};


class Metal : public Material
{
public:
    Metal(const Vec3& a_oVecIn, float a_fFuzz);

    virtual bool Scatter(const Ray &a_oRayIn, const HitRecord &a_oRecord, Vec3 &a_oAttenuation, Ray &a_oScatterRay) const;

    Vec3 m_oAlbedo;

    float m_fFuzz;
};

Metal::Metal(const Vec3 &a_oVecIn, float a_fFuzz) : m_oAlbedo(a_oVecIn)
{
    if (a_fFuzz < 1)
    {
        m_fFuzz = a_fFuzz;
    }
    else
    {
        m_fFuzz = 1.0f;
    }
}

bool Metal::Scatter(const Ray &a_oRayIn, const HitRecord &a_oRecord, Vec3 &a_oAttenuation, Ray &a_oScatterRay) const
{
    Vec3 _reflected = Reflect(Unit_Vector(a_oRayIn.Direction()), a_oRecord.m_oNormal);
    a_oScatterRay = Ray(a_oRecord.m_oPoint, _reflected + m_fFuzz * RandomInUnitSphere());
    a_oAttenuation = m_oAlbedo;
    return (Dot(a_oScatterRay.Direction(), a_oRecord.m_oNormal) > 0);
}

class Lambertian : public Material
{
public:
    Lambertian(const Vec3& a_oVecIn);

    virtual bool Scatter(const Ray &a_oRayIn, const HitRecord &a_oRecord, Vec3 &a_oAttenuation, Ray &a_oScatterRay) const;

    Vec3 m_oAlbedo;
};

Lambertian::Lambertian(const Vec3& a_oVecIn) : m_oAlbedo(a_oVecIn)
{
}

bool Lambertian::Scatter(const Ray &a_oRayIn, const HitRecord &a_oRecord, Vec3 &a_oAttenuation, Ray &a_oScatterRay) const
{
    Vec3 _target = a_oRecord.m_oPoint + a_oRecord.m_oNormal + RandomInUnitSphere();

    a_oScatterRay = Ray(a_oRecord.m_oPoint, _target - a_oRecord.m_oPoint);

    a_oAttenuation = m_oAlbedo;

    return true;
}

class Dielectric : public Material
{
public:
    Dielectric(float a_fRefractionIdx);

    virtual bool Scatter(const Ray &a_oRayIn, const HitRecord &a_oRecord, Vec3 &a_oAttenuation, Ray &a_oScatterRay) const;

    float m_fRefIdx;
};

Dielectric::Dielectric(float a_fRefractionIdx) : m_fRefIdx(a_fRefractionIdx)
{
}

bool Dielectric::Scatter(const Ray &a_oRayIn, const HitRecord &a_oRecord, Vec3 &a_oAttenuation, Ray &a_oScatterRay) const
{
    Vec3 _outwardNormal;

    Vec3 _reflected = Reflect(a_oRayIn.Direction(), a_oRecord.m_oNormal);

    float _niOverNt;

    a_oAttenuation = Vec3(1.0f, 1.0f, 1.0f);

    Vec3 _refracted;

    float _reflectProb;

    float _cosine;

    if (Dot(a_oRayIn.Direction(), a_oRecord.m_oNormal) > 0)
    {
        _outwardNormal = -a_oRecord.m_oNormal;

        _niOverNt = m_fRefIdx;

        _cosine = Dot(a_oRayIn.Direction(), a_oRecord.m_oNormal) / a_oRayIn.Direction().Length();

        _cosine = sqrtf(1.0f - m_fRefIdx * m_fRefIdx * (1.0f - _cosine * _cosine));
    }
    else
    {
        _outwardNormal = a_oRecord.m_oNormal;

        _niOverNt = 1.0f / m_fRefIdx;

        _cosine = -Dot(a_oRayIn.Direction(), a_oRecord.m_oNormal) / a_oRayIn.Direction().Length();
    }

    if (Refract(a_oRayIn.Direction(), _outwardNormal, _niOverNt, _refracted))
    {
        _reflectProb = Schlick(_cosine, m_fRefIdx);
    }
    else
    {
        _reflectProb = 1.0f;
    }

    if ((std::rand() / (RAND_MAX + 1.0)) < _reflectProb)
    {
        a_oScatterRay = Ray(a_oRecord.m_oPoint, _reflected);
    }
    else
    {
        a_oScatterRay = Ray(a_oRecord.m_oPoint, _refracted);
    }

    return true;
}

#endif // MATERIAL_H
