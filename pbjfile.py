#!/usr/bin/env python3
#############################################################################
# pbjfile.py                                                                #
#############################################################################
#                         This file is part of:                             #
#                           DABOZZ ENGINE                                   #
#############################################################################
# Copyright (c) 2026-present DabozzEngine contributors.                     #
#                                                                           #
# PB&J build configuration for DabozzEditor.                                #
#############################################################################

from pbj import Environment

env = Environment()

env.project_name = "DabozzEditor"
env.compiler = "C:/Qt/Tools/mingw1310_64/bin/g++.exe"
env.linker = "C:/Qt/Tools/mingw1310_64/bin/g++.exe"
env.output = "DabozzEditor.exe"
env.obj_dir = "obj"
env.bin_dir = "bin"

## Qt Tools #################################################################

env.moc_path = "C:/Qt/6.10.2/mingw_64/bin/moc.exe"
env.rcc_path = "C:/Qt/6.10.2/mingw_64/bin/rcc.exe"
env.add_moc_dirs(["include"])
env.add_qrc("shaders.qrc")

## Sources ##################################################################

env.add_sources("src", extensions=[".cpp", ".c"])

## Includes #################################################################

env.add_includes([
    "include",
    "assimp_source/include",
    "assimp_build/include",
    "assimp_source/contrib/stb",
    "glm",
    "openal-soft/include",
    "C:/Program Files/Mono/include/mono-2.0",
    "C:/Qt/6.10.2/mingw_64/include",
    "C:/Qt/6.10.2/mingw_64/include/QtOpenGLWidgets",
    "C:/Qt/6.10.2/mingw_64/include/QtWidgets",
    "C:/Qt/6.10.2/mingw_64/include/QtOpenGL",
    "C:/Qt/6.10.2/mingw_64/include/QtGui",
    "C:/Qt/6.10.2/mingw_64/include/QtCore",
])

## Compiler Flags ###########################################################

env.add_cflags([
    "-std=gnu++1z",
    "-Wall",
    "-Wextra",
    "-fexceptions",
    "-mthreads",
])

env.add_defines([
    "UNICODE",
    "_UNICODE",
    "WIN32",
    "MINGW_HAS_SECURE_API=1",
    "QT_NO_DEBUG",
    "QT_OPENGLWIDGETS_LIB",
    "QT_WIDGETS_LIB",
    "QT_OPENGL_LIB",
    "QT_GUI_LIB",
    "QT_CORE_LIB",
    "QT_NEEDS_QMAIN",
])

## Linker ###################################################################

env.add_ldflags([
    "-Wl,-s",
    "-Wl,-subsystem,windows",
    "-mthreads",
])

env.add_lib_dirs([
    "C:/Qt/6.10.2/mingw_64/lib",
    "C:/Program Files/Mono/lib",
    "assimp_build/bin",
    "openal-soft/build",
])

env.add_ldflags([
    "C:/Qt/6.10.2/mingw_64/lib/libQt6OpenGLWidgets.a",
    "C:/Qt/6.10.2/mingw_64/lib/libQt6Widgets.a",
    "C:/Qt/6.10.2/mingw_64/lib/libQt6OpenGL.a",
    "C:/Qt/6.10.2/mingw_64/lib/libQt6Gui.a",
    "C:/Qt/6.10.2/mingw_64/lib/libQt6Core.a",
])

env.add_libs([
    "mingw32",
    "shell32",
    "opengl32",
    "glu32",
    "assimp-5",
    "mono-2.0-sgen",
    "OpenAL32",
])

env.add_ldflags([
    "C:/Qt/6.10.2/mingw_64/lib/libQt6EntryPoint.a",
    "assimp_build/bin/libassimp-5.dll",
])

## Deploy ###################################################################

QT_BIN = "C:/Qt/6.10.2/mingw_64/bin"
MINGW_BIN = "C:/Qt/Tools/mingw1310_64/bin"

for dll in ["Qt6Core", "Qt6Gui", "Qt6Widgets", "Qt6OpenGL", "Qt6OpenGLWidgets", "Qt6Svg"]:
    env.deploy(f"{QT_BIN}/{dll}.dll")

for dll in ["libgcc_s_seh-1", "libstdc++-6", "libwinpthread-1"]:
    env.deploy(f"{MINGW_BIN}/{dll}.dll")

env.deploy("assimp_build/bin/libassimp-5.dll")
env.deploy("openal-soft/build/OpenAL32.dll")

env.deploy_dir(f"{QT_BIN}/../plugins/platforms", "platforms")
