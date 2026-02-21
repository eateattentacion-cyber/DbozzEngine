# DabozzEngine

A lightweight, open-source game engine built from scratch in C++ with OpenGL and a full Qt6 editor.

## Features

- **Entity Component System** — Flexible template-based ECS for organizing game objects
- **PBR Renderer** — Physically based rendering with OpenGL, real-time lighting, skybox support
- **Physics Engine (Butsuri)** — Rigid body simulation, box/sphere colliders, gravity, collision detection
- **Animation System** — Skeletal animation with cross-fade blending, visual state machine editor, SLERP interpolation
- **Audio System** — OpenAL-powered audio with WAV playback, volume/pitch control, spatial sound
- **Dual Scripting System** — Lua and AngelScript support with comprehensive API, syntax highlighting, and auto-save
- **Full Editor** — Dark-themed Qt6 editor with hierarchy, inspector, asset browser, script editor, undo/redo
- **Scene Management** — JSON-based scene serialization with Godot-style project manager
- **Model Import** — OBJ, FBX, GLTF, GLB, DAE support via Assimp
- **PB&J Build System** — Custom Python build system with incremental compilation and parallel builds

## Scripting

DabozzEngine supports both Lua and AngelScript for game logic:

### Lua Example
```lua
function Start()
    local cube = CreateEntity()
    SetEntityName(cube, "Player")
    SetEntityPosition(cube, 0.0, 5.0, 0.0)
    CreateCube(cube, 2.0)
    AddRigidbody(cube, 1.0, false)
    AddBoxCollider(cube, 2.0, 2.0, 2.0)
end

function Update(deltaTime)
end
```

### AngelScript Example
```angelscript
void Start()
{
    uint cube = CreateEntity();
    SetEntityName(cube, "Player");
    SetEntityPosition(cube, 0.0f, 5.0f, 0.0f);
    CreateCube(cube, 2.0f);
    AddRigidbody(cube, 1.0f, false);
    AddBoxCollider(cube, 2.0f, 2.0f, 2.0f);
}

void Update(float deltaTime)
{
}
```

### Scripting API
- Entity management (Create, Destroy, FindByName, SetName, GetName)
- Transform operations (Position, Rotation, Scale)
- Physics (AddRigidbody, SetVelocity, ApplyForce, SetGravity)
- Colliders (AddBoxCollider, AddSphereCollider)
- Rendering (LoadMesh, CreateCube)
- Math utilities (Distance, Lerp, LookAt)
- Input (IsKeyPressed, IsKeyDown, IsMouseButtonPressed)
- Scene management (LoadScene, SaveScene)

## Building

### Requirements
- C++ compiler (MinGW 13+)
- Qt 6.10+
- Python 3
- CMake

### Build Steps

```bash
git clone --recursive https://github.com/eateattentacion-cyber/DbozzEngine.git
cd DbozzEngine

cd assimp_source
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DASSIMP_WARNINGS_AS_ERRORS=OFF
mingw32-make -j8
cd ../..

mkdir openal-soft/build && cd openal-soft/build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
mingw32-make -j8
cd ../..

build_lua.bat

cd angelscript/sdk/angelscript/projects/mingw
mingw32-make
cd ../../../../..

python pbj.py build --target release

./bin/DabozzEditor.exe
```

## Project Structure

```
DbozzEngine/
├── src/              # Engine source code
├── include/          # Engine headers
├── assets/           # Default assets
├── angelscript/      # AngelScript submodule
├── lua/              # Lua submodule
├── assimp_source/    # Assimp library
├── openal-soft/      # OpenAL library
└── bin/              # Compiled binaries
```

## License

Open source. See repository for details.
