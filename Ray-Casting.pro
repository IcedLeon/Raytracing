TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    appsrc/src/Math/vec3.cpp \
    appsrc/src/Math/ray.cpp \
    appsrc/src/Math/sphere.cpp \
    appsrc/src/Math/hittablelist.cpp \
    appsrc/src/Math/camera.cpp

HEADERS += \
    appsrc/include/Math/vec3.h \
    appsrc/include/Math/ray.h \
    appsrc/include/Math/hittable.h \
    appsrc/include/Math/sphere.h \
    appsrc/include/Math/hittablelist.h \
    appsrc/include/Math/camera.h \
    appsrc/include/Math/material.h
