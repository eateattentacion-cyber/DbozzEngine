QT += core widgets opengl openglwidgets
CONFIG += c++17
TARGET = DabozzEditor
TEMPLATE = app

SOURCES += \
    src/main.cpp \
    src/editor/mainwindow.cpp \
    src/editor/sceneview.cpp \
    src/editor/gamewindow.cpp \
    src/editor/componentinspector.cpp \
    src/editor/hierarchyview.cpp \
    src/renderer/openglrenderer.cpp \
    src/renderer/meshloader.cpp \
    src/ecs/world.cpp \
    src/physics/butsuri.cpp \
    src/physics/physicssystem.cpp

HEADERS += \
    include/editor/mainwindow.h \
    include/editor/sceneview.h \
    include/editor/gamewindow.h \
    include/editor/componentinspector.h \
    include/editor/hierarchyview.h \
    include/renderer/openglrenderer.h \
    include/renderer/meshloader.h \
    include/ecs/world.h \
    include/ecs/component.h \
    include/ecs/entity.h \
    include/ecs/system.h \
    include/ecs/components/transform.h \
    include/ecs/components/name.h \
    include/ecs/components/hierarchy.h \
    include/ecs/components/mesh.h \
    include/ecs/components/firstpersoncontroller.h \
    include/ecs/components/collider.h \
    include/ecs/components/boxcollider.h \
    include/ecs/components/spherecollider.h \
    include/ecs/components/floorcollider.h \
    include/ecs/components/rigidbody.h \
    include/physics/simplephysics.h \
    include/physics/physicssystem.h \
    include/debug/logger.h

RESOURCES += shaders.qrc

INCLUDEPATH += include

# Assimp
INCLUDEPATH += $$PWD/assimp_source/include
INCLUDEPATH += $$PWD/assimp_build/include
LIBS += $$PWD/assimp_build/bin/libassimp-5.dll

# STB Image (from assimp contrib)
INCLUDEPATH += $$PWD/assimp_source/contrib/stb
