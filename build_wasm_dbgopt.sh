emsdk activate mingw-7.1.0-64bit
emcmake cmake -S . -B build-web -D CMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-web
