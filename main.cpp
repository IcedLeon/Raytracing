#include <iostream>
#include <fstream>
#include <float.h>
#include <string>
#include <stdlib.h>
#include "appsrc/include/Math/sphere.h"
#include "appsrc/include/Math/hittablelist.h"
#include "appsrc/include/Math/camera.h"
#include "appsrc/include/Math/material.h"

#define M_PI 3.14159265358979323846

Vec3 Color(const Ray &a_oRay, Hittable *a_oWorld, int a_iDepth)
{
    HitRecord _record;

    if (a_oWorld->Hit(a_oRay, 0.001f, FLT_MAX, _record))
    {
        Ray _scatter;
        Vec3 _attenuation;

        if (a_iDepth < 50 && _record.m_oMaterial->Scatter(a_oRay, _record, _attenuation, _scatter))
        {
            return _attenuation * Color(_scatter, a_oWorld, a_iDepth + 1);
        }
        else
        {
            return Vec3(0.0f, 0.0f, 0.0f);
        }
    }
    else
    {
        Vec3 _unitDir = Unit_Vector(a_oRay.Direction());

        float _t = 0.5f * (_unitDir.GetY() + 1.0f);

        return (1.0f - _t) * Vec3(1.0f, 1.0f, 1.0f) + _t * Vec3(0.5f, 0.7f, 1.0f);
    }
}

Hittable *RandomScene()
{
    int n = 500;

    Hittable **_list = new Hittable *[n + 1];

    _list[0] = new Sphere(Vec3(0.0f, -1000.0f, 0.0f), 1000.0f, new Lambertian(Vec3(0.5f, 0.5f, 0.5f)));

    int i = 1;

    for (int a = -11; a < 11; ++a)
    {
        for (int b = -11; b < 11; ++b)
        {
            float _chooseMat = (std::rand() / (RAND_MAX + 1.0));

            Vec3 _center(a + 0.9f * (std::rand() / (RAND_MAX + 1.0)), 0.2f, b + 0.9f * (std::rand() / (RAND_MAX + 1.0)));

            if ((_center - Vec3(4.0f, 0.2f, 0.0f)).Length() > 0.9f)
            {
                if (_chooseMat < 0.8f)
                {
                    _list[i++] = new Sphere(_center, 0.2f, new Lambertian(Vec3((std::rand() / (RAND_MAX + 1.0)) * (std::rand() / (RAND_MAX + 1.0)), (std::rand() / (RAND_MAX + 1.0)) * (std::rand() / (RAND_MAX + 1.0)), (std::rand() / (RAND_MAX + 1.0)) * (std::rand() / (RAND_MAX + 1.0)))));
                }
                else if (_chooseMat < 0.95f)
                {
                    _list[i++] = new Sphere(_center, 0.2f,
                                            new Metal(Vec3(0.5f * (1.0f + (std::rand() / (RAND_MAX + 1.0))), 0.5f * (1.0f + (std::rand() / (RAND_MAX + 1.0))), 0.5f * (1.0f + (std::rand() / (RAND_MAX + 1.0)))), 0.5f * (std::rand() / (RAND_MAX + 1.0))));
                }
                else
                {
                    _list[i++] = new Sphere(_center, 0.2f, new Dielectric(1.5f));
                }
            }
        }
    }

    _list[i++] = new Sphere(Vec3(0.0f, 1.0f, 0.0f), 1.0f, new Dielectric(1.5f));
    _list[i++] = new Sphere(Vec3(-4.0f, 1.0f, 0.0f), 1.0f, new Lambertian(Vec3(0.4f, 0.2f, 0.1f)));
    _list[i++] = new Sphere(Vec3(4.0f, 1.0f, 0.0f), 1.0f, new Metal(Vec3(0.7f, 0.6f, 0.5f), 0.0f));

    return new HittableList(_list, i);
}

int main(int argc, char const *argv[])
{
    int nx = 400;
    int ny = 300;
    int ns = 5;

    std::fstream _outputFile;

    _outputFile.open("output.ppm");

    std::string _head = "P3\n" + std::to_string(nx) + " " + std::to_string(ny) + "\n255\n";

    _outputFile << _head;

    Hittable *_list[5];

    float _r = static_cast<float>(cos(M_PI / 4));

    _list[0] = new Sphere(Vec3(0.0f, 0.0f, -1.0f), 0.5f, new Lambertian(Vec3(0.1f, 0.2f, 0.5f)));
    _list[1] = new Sphere(Vec3(0.0f, -100.5f, -1.0f), 100, new Lambertian(Vec3(0.8f, 0.8f, 0.0f)));
    _list[2] = new Sphere(Vec3(1.0f, 0.0f, -1.0f), 0.5f, new Metal(Vec3(0.8f, 0.6f, 0.2f), 0.0));
    _list[3] = new Sphere(Vec3(-1.0f, 0.0f, -1.0f), 0.5f, new Dielectric(1.5f));
    _list[3] = new Sphere(Vec3(-1.0f, 0.0f, -1.0f), -0.45f, new Dielectric(1.5f));

    Hittable *_world = RandomScene();

    Vec3 _lookFrom(13.0f, 2.0f, 3.0f);
    Vec3 _lookAt(0.0f, 0.0f, 0.0f);
    float _distToFocus = 10.0f;
    float _aperture = 0.1f;

    Camera _camera(_lookFrom, _lookAt, Vec3(0.0f, 1.0f, 0.0f), 20.0f, float(nx) / float(ny), _aperture, _distToFocus);

    for (int j = ny - 1; j >= 0; --j)
    {
        for (int i = 0; i < nx; ++i)
        {
            Vec3 _col(0.0f, 0.0f, 0.0f);

            for (int s = 0; s < ns; ++s)
            {
                float _u = float(i + (std::rand() / (RAND_MAX + 1.0))) / float(nx);
                float _v = float(j + (std::rand() / (RAND_MAX + 1.0))) / float(ny);

                Ray _ray = _camera.GetRay(_u, _v);

                Vec3 _p = _ray.PointAtParamenter(2.0f);

                _col += Color(_ray, _world, 0);
            }
            _col /= float(ns);
            _col = Vec3(sqrtf(_col[0]), sqrtf(_col[1]), sqrtf(_col[2]));

            int ir = int(255.99 * _col[0]);
            int ig = int(255.99 * _col[1]);
            int ib = int(255.99 * _col[2]);

            // std::cout << ir << " " << ig << " " << ib << "\n";
            std::string _row = std::to_string(ir) + " " + std::to_string(ig) + " " + std::to_string(ib) + "\n";
            _outputFile << _row;
        }
    }
    _outputFile.close();
    return 0;
}
