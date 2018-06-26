cmake_minimum_required(VERSION 2.8.4)
project(verifypn)

if (UNIX AND NOT APPLE)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8) # is system 64-bit?
        set(ARCH_TYPE "linux64")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -flto -march=x86-64 -std=c++14  -m64 -I.")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto=4 -march=x86-64 -std=c++14 -m64 -static -static-libgcc -static-libstdc++")
    else()
        set(ARCH_TYPE "linux32")
    endif ()
elseif(APPLE)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8) # is system 64-bit?
        set(ARCH_TYPE "osx64")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=10.7 -std=c++14  -m64 -I. -stdlib=libc++")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -dynamic -mmacosx-version-min=10.7 -std=c++14 -m64 -stdlib=libc++ -lc++")
    else()
        set(ARCH_TYPE "osx32")
    endif ()
else()
    if (CMAKE_CL_64)
        set(ARCH_TYPE "win64")
    else()
        set(ARCH_TYPE "win32")
    endif()
endif ()

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -pedantic-errors -O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS_DEBUG} -g")

set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -O2 -DNDEBUG")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -g")

include_directories(PUBLIC .)

file(GLOB verifypn_SRC
        "*.h"
        "*.cpp"
        "**/*.cpp"
        "**/*.h"
        "**/**/*.cpp"
        "**/**/*.h"
)

add_executable(verifypn ${verifypn_SRC})
target_link_libraries(verifypn ${CMAKE_SOURCE_DIR}/lpsolve/liblpsolve55-${ARCH_TYPE}.a)
