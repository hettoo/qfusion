# cmake -DCMAKE_TOOLCHAIN_FILE=cmake/i686-mingw.cmake .
set(CMAKE_SYSTEM_NAME "Windows")

SET(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
SET(CMAKE_CXX_COMPILER i686-w64-mingw32-g++)
SET(CMAKE_RC_COMPILER i686-w64-mingw32-windres)
# SET(CMAKE_AR i686-w64-mingw32-ar)
# SET(CMAKE_RANLIB i686-w64-mingw32-ranlib)