# DabozzEngine

A lightweight, open-source game engine built from scratch in C++ with OpenGL and a full Qt6 editor.

## Features

- **Entity Component System** — Flexible template-based ECS for organizing game objects
- **PBR Renderer** — Physically based rendering with OpenGL, real-time lighting, skybox support
- **Physics Engine (Butsuri)** — Rigid body simulation, box/sphere colliders, gravity, collision detection
- **Animation System** — Skeletal animation with cross-fade blending, visual state machine editor, SLERP interpolation
- **Audio System** — OpenAL-powered audio with WAV playback, volume/pitch control, spatial sound
- **C# Scripting** — Mono-powered scripting with built-in editor and syntax highlighting
- **Full Editor** — Dark-themed Qt6 editor with hierarchy, inspector, asset browser, undo/redo
- **Scene Management** — JSON-based scene serialization with project system
- **Model Import** — OBJ, FBX, GLTF, GLB, DAE support via Assimp
- **PB&J Build System** — Custom Python build system with incremental compilation and parallel builds

## Building

### Requirements
- C++ compiler (MinGW 13+)
- Qt 6.10+
- Python 3
- CMake

### Build Steps

```bash
# Clone
git clone https://github.com/eateattentacion-cyber/DbozzEngine.git
cd DbozzEngine

# Build OpenAL
cd openal-soft/build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
mingw32-make -j8
cd ../..

# Build the engine
python pbj.py build --target release

# Build the hub
cd dabozzhub
python pbj.py build --target release
cd ..

# Run
./bin/DabozzHub.exe
```

## Screenshots

*Coming soon*

## License

Open source. See repository for details.
