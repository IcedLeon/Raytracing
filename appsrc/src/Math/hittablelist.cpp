#include "appsrc/include/Math/hittablelist.h"

HittableList::HittableList() : m_oList(nullptr),
                               m_iListSize(0)
{
}

HittableList::HittableList(Hittable **a_oData, int a_iN) : m_oList(a_oData),
                                                           m_iListSize(a_iN)
{
}

bool HittableList::Hit(const Ray &a_oRay, float a_fTMin, float a_fTMax, HitRecord &a_oRecord) const
{
    HitRecord _tempRecord;

    bool _hittedAnything = false;

    double _closestSoFar = a_fTMax;

    for (int i = 0; i < m_iListSize; ++i)
    {
        if (m_oList[i]->Hit(a_oRay, a_fTMin, _closestSoFar, _tempRecord))
        {
            _hittedAnything = true;
            _closestSoFar = _tempRecord.m_fT;
            a_oRecord = _tempRecord;
        }
    }
    return _hittedAnything;
}
