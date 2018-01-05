QT += gui core

CONFIG += c++11

TARGET = PaintSpatter
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH += $$PWD/../glm/glm

SOURCES += main.cpp \
    PaintSpatter.cpp \
    teapot.cpp \
    vboplane.cpp

HEADERS += \
    PaintSpatter.h \
    teapotdata.h \
    teapot.h \
    vboplane.h

OTHER_FILES += \
    fshader.txt \
    vshader.txt

RESOURCES += \
    shaders.qrc

DISTFILES += \
    fshader.txt \
    vshader.txt
