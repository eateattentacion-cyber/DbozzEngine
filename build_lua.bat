@echo off
cd lua
for %%f in (*.c) do (
    if not "%%f"=="lua.c" if not "%%f"=="luac.c" if not "%%f"=="onelua.c" gcc -O2 -c %%f
)
ar rcs liblua.a *.o
move liblua.a ..
del *.o
cd ..
