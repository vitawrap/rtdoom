emsdk activate mingw-7.1.0-64bit
emcmake cmake -S . -B build-web -D CMAKE_BUILD_TYPE=Release
cmake --build build-web
