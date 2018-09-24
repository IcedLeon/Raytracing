#ifndef HITTABLELIST_H
#define HITTABLELIST_H

#include "appsrc/include/Math/hittable.h"

class HittableList : public Hittable
{
public:
    HittableList();
    HittableList(Hittable** a_oData, int a_iN);

    virtual bool Hit(const Ray &a_oRay, float a_fTMin, float a_fTMax, HitRecord &a_oRecord) const;

    Hittable** m_oList;
    int m_iListSize;
};

#endif // HITTABLELIST_H
