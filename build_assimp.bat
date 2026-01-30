@echo off
setlocal

REM Set MinGW paths
set MINGW_PATH=C:\Qt\Tools\mingw1310_64
set PATH=%MINGW_PATH%\bin;%PATH%

REM Create build directory
if not exist "assimp_build" mkdir assimp_build
cd assimp_build

REM Configure with CMake
cmake -G "MinGW Makefiles" ../assimp_source ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_CXX_FLAGS="-Wno-error=unknown-pragmas" ^
    -DCMAKE_C_FLAGS="-Wno-error=unknown-pragmas" ^
    -DASSIMP_BUILD_TESTS=OFF ^
    -DASSIMP_BUILD_ASSIMP_TOOLS=OFF ^
    -DASSIMP_BUILD_SAMPLES=OFF ^
    -DASSIMP_BUILD_ZLIB=ON ^
    -DASSIMP_INSTALL=OFF ^
    -DASSIMP_WARNINGS_AS_ERRORS=OFF

if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed!
    cd ..
    pause
    exit /b 1
)

REM Build
mingw32-make -j%NUMBER_OF_PROCESSORS%

if %ERRORLEVEL% neq 0 (
    echo Build failed!
    cd ..
    pause
    exit /b 1
)

echo Build completed!
cd ..
pause
