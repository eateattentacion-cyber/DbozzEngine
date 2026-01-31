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
    src/editor/animatorgrapheditor.cpp \
    src/editor/graphitems/statenodeitem.cpp \
    src/editor/graphitems/transitionarrowitem.cpp \
    src/editor/graphitems/entrynodeitem.cpp \
    src/renderer/openglrenderer.cpp \
    src/renderer/meshloader.cpp \
    src/renderer/animation.cpp \
    src/renderer/skeleton.cpp \
    src/ecs/world.cpp \
    src/ecs/animatorgraph.cpp \
    src/physics/butsuri.cpp \
    src/physics/physicssystem.cpp \
    src/scripting/scriptingengine.cpp \
    src/scripting/scriptinternalcalls.cpp \
    src/editor/scripteditor.cpp

HEADERS += \
    include/editor/mainwindow.h \
    include/editor/sceneview.h \
    include/editor/gamewindow.h \
    include/editor/componentinspector.h \
    include/editor/hierarchyview.h \
    include/editor/animatorgrapheditor.h \
    include/editor/graphitems/statenodeitem.h \
    include/editor/graphitems/transitionarrowitem.h \
    include/editor/graphitems/entrynodeitem.h \
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
    include/ecs/components/animator.h \
    include/ecs/components/animatorgraph.h \
    include/ecs/systems/animationsystem.h \
    include/renderer/animation.h \
    include/renderer/skeleton.h \
    include/physics/simplephysics.h \
    include/physics/physicssystem.h \
    include/scripting/scriptingengine.h \
    include/scripting/scriptinternalcalls.h \
    include/editor/scripteditor.h \
    include/debug/logger.h

RESOURCES += shaders.qrc

INCLUDEPATH += include

# Assimp
INCLUDEPATH += $$PWD/assimp_source/include
INCLUDEPATH += $$PWD/assimp_build/include
LIBS += $$PWD/assimp_build/bin/libassimp-5.dll

# STB Image (from assimp contrib)
INCLUDEPATH += $$PWD/assimp_source/contrib/stb

# GLM
INCLUDEPATH += $$PWD/glm

# Mono
INCLUDEPATH += "C:/Program Files/Mono/include/mono-2.0"
LIBS += -L"C:/Program Files/Mono/lib" -lmono-2.0-sgen
