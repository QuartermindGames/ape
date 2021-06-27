mkdir build\msvc2019-x64
pushd build\msvc2019-x64
cmake -G "Visual Studio 16 2019" -A x64 %* ../..
popd
pause