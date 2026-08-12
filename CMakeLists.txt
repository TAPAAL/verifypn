cmake_minimum_required(VERSION 3.31)

set(DEFAULT_GCC_VERSION 14)

## Default to release builds, must be set before project
if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

# Project Definition
set(VERIFYPN_NAME "verifypn")
set(VERIFYPN_VERSION 4.4.2)

# Select GCC compiler
set(DESIRED_GCC_VERSION "$ENV{GCC_VERSION}")
if (DESIRED_GCC_VERSION STREQUAL "")
    set(DESIRED_GCC_VERSION ${DEFAULT_GCC_VERSION})
endif()

if (NOT CMAKE_C_COMPILER)
    find_program(GCC_REQ_C NAMES "gcc-${DESIRED_GCC_VERSION}" gcc REQUIRED)
    set(CMAKE_C_COMPILER "${GCC_REQ_C}" CACHE STRING "C compiler")
endif()

if (NOT CMAKE_CXX_COMPILER)
    find_program(GCC_REQ_CXX NAMES "g++-${DESIRED_GCC_VERSION}" g++ REQUIRED)
    set(CMAKE_CXX_COMPILER "${GCC_REQ_CXX}" CACHE STRING "CXX compiler")
endif()

project(${VERIFYPN_NAME} VERSION ${VERIFYPN_VERSION} LANGUAGES CXX C)

if (NOT CMAKE_C_COMPILER_ID STREQUAL "GNU" OR NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "GCC is required, but found ${CMAKE_C_COMPILER_ID} and ${CMAKE_CXX_COMPILER_ID}")
endif()

if (NOT CMAKE_CROSSCOMPILING)
    if (NOT CMAKE_C_COMPILER_VERSION MATCHES "^${DESIRED_GCC_VERSION}([.].*)?$")
        message(FATAL_ERROR "GCC ${DESIRED_GCC_VERSION} is required, but found GCC ${CMAKE_C_COMPILER_VERSION}")
    endif()
    if (NOT CMAKE_CXX_COMPILER_VERSION MATCHES "^${DESIRED_GCC_VERSION}([.].*)?$")
        message(FATAL_ERROR "G++ ${DESIRED_GCC_VERSION} is required, but found G++ ${CMAKE_CXX_COMPILER_VERSION}")
    endif()
endif()

# C++ Definition
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Default Values
## Build Flags
option(VERIFYPN_Static "Link libraries statically" ON)
option(VERIFYPN_GetDependencies "Fetch external dependencies from web." ON)
set(EXTERNAL_INSTALL_LOCATION ${CMAKE_BINARY_DIR}/external CACHE PATH "Install location for external dependencies")
option(VERIFYPN_MC_Simplification "Enables multicore simplification, incompatible with static linking" OFF)
option(VERIFYPN_TEST "Build unit tests" OFF)
set(VERIFYPN_TARGETDIR "${CMAKE_BINARY_DIR}/${VERIFYPN_NAME}" CACHE PATH "Traget directory for build files")
set(VERIFYPN_OSX_DEPLOYMENT_TARGET 10.8 CACHE STRING "Specify the minimum version of the target platform for MacOS on which the target binaries are to be deployed ")

# Configure Project

## Configure Static
if (VERIFYPN_Static)
    set(BUILD_SHARED_LIBS OFF)
    if (NOT APPLE)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    else ()
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    endif()
endif (VERIFYPN_Static)

## Configure Traget Platform
if (UNIX AND NOT APPLE)
    set(ARCH_TYPE "linux64")
elseif (APPLE)
    set(ARCH_TYPE "osx64")
    set(CMAKE_OSX_DEPLOYMENT_TARGET ${VERIFYPN_OSX_DEPLOYMENT_TARGET})
else ()
    set(ARCH_TYPE "win64")
endif ()

## Enabled INTERPROCEDURAL_OPTIMIZATION for non-macos
if (CMAKE_BUILD_TYPE MATCHES Release)
    if (NOT APPLE AND NOT WIN32)
    	set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif ()
endif ()

## CXX Flags
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s -Wall -pedantic-errors -O3 -DNDEBUG")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -Wall -pedantic-errors -fno-omit-frame-pointer ") # also usefull : -fsanitize=address -fsanitize=undefined -fsanitize=leak

## Target File location
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${VERIFYPN_TARGETDIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${VERIFYPN_TARGETDIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${VERIFYPN_TARGETDIR}/bin)

# Required Packages and Tools

find_package(FLEX 2.6.4 REQUIRED)
find_package(BISON 3.0.5 REQUIRED)

if (VERIFYPN_GetDependencies)
    if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
        cmake_policy(SET CMP0135 NEW)
    endif()

    include(ExternalProject)
    include(ProcessorCount)
    ProcessorCount(DEPENDENCY_JOBS)
    if (NOT DEPENDENCY_JOBS)
        set(DEPENDENCY_JOBS 1)
    endif()
    # Install location should be ${EXTERNAL_INSTALL_LOCATION}

    ExternalProject_add(ptrie-ext
        GIT_REPOSITORY https://github.com/petergjoel/ptrie
        GIT_TAG 9132d723cf19fad5a402f4b19a194c6aa66325fb
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_LOCATION} -DPTRIE_BuildTest=OFF -DCMAKE_BUILD_TYPE=Release
    )
    ExternalProject_add(rapidxml-ext
        URL https://downloads.sourceforge.net/project/rapidxml/rapidxml/rapidxml%201.13/rapidxml-1.13.zip
        URL_HASH SHA512=6c10583e6631ccdb0217d0a5381172cb4c1046226de6ef1acf398d85e81d145228e14c3016aefcd7b70a1db8631505b048d8b4f5d4b0dbf1811d2482eefdd265
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        BUILD_COMMAND ""
        CONFIGURE_COMMAND mkdir -p ${CMAKE_BINARY_DIR}/external/include
        INSTALL_COMMAND cd ../rapidxml-ext && ${CMAKE_COMMAND} -E copy rapidxml.hpp rapidxml_iterators.hpp rapidxml_print.hpp rapidxml_utils.hpp ${EXTERNAL_INSTALL_LOCATION}/include
    )

    if (WIN32) #If windows 32 or 64
      set(GLPK_CFLAGS "-D __WOE__ -O3 -std=gnu17" )
      set(HOST "--host=x86_64-w64-mingw32")
      message("Windows")
    else(WIN32)
      set(GLPK_CFLAGS "-O3 -std=gnu17" )
    endif(WIN32)

    if (UNIX AND NOT APPLE)
        ExternalProject_add(spot-ext
            URL http://www.lrde.epita.fr/dload/spot/spot-2.11.3.tar.gz
            URL_HASH SHA512=bb838b5a5eb772f31f1e39389d27f5e6b478713040cd6609a76c534afb491bc477d54f7fa8c7c4800efae74259781fddeff2854f601b685745797faaf7998d20
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            BUILD_COMMAND make -j${DEPENDENCY_JOBS} -C lib COMMAND make -j${DEPENDENCY_JOBS} -C buddy COMMAND make -j${DEPENDENCY_JOBS} -C picosat COMMAND make -j${DEPENDENCY_JOBS} -C ltdl COMMAND make -j${DEPENDENCY_JOBS} -C spot
            CONFIGURE_COMMAND CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} <SOURCE_DIR>/configure --prefix ${EXTERNAL_INSTALL_LOCATION} --disable-python --disable-devel --disable-debug --disable-shared --enable-static --enable-c++17 CFLAGS=-flto CXXFLAGS=-flto LDFLAGS=-fuse-linker-plugin
            INSTALL_COMMAND make -C lib install COMMAND make -C buddy install COMMAND make -C picosat install COMMAND make -C ltdl install COMMAND make -C spot install
            PATCH_COMMAND patch -p1 < ${PROJECT_SOURCE_DIR}/spot-gcc16.patch
        )
    elseif (APPLE)
        ExternalProject_add(spot-ext
            URL http://www.lrde.epita.fr/dload/spot/spot-2.11.3.tar.gz
            URL_HASH SHA512=bb838b5a5eb772f31f1e39389d27f5e6b478713040cd6609a76c534afb491bc477d54f7fa8c7c4800efae74259781fddeff2854f601b685745797faaf7998d20
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            BUILD_COMMAND make -j${DEPENDENCY_JOBS} -C lib COMMAND make -j${DEPENDENCY_JOBS} -C buddy COMMAND make -j${DEPENDENCY_JOBS} -C picosat COMMAND make -j${DEPENDENCY_JOBS} -C ltdl COMMAND make -j${DEPENDENCY_JOBS} -C spot
            CONFIGURE_COMMAND CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} <SOURCE_DIR>/configure --prefix ${EXTERNAL_INSTALL_LOCATION} --disable-python --disable-devel --disable-debug --disable-shared --enable-static --enable-c++17 CFLAGS=${CMAKE_CXX_FLAGS} LDFLAGS=${CMAKE_LINKER_FLAGS}
            INSTALL_COMMAND make -C lib install COMMAND make -C buddy install COMMAND make -C picosat install COMMAND make -C ltdl install COMMAND make -C spot install
            PATCH_COMMAND patch -p1 < ${PROJECT_SOURCE_DIR}/spot-gcc16.patch
        )
    else ()
        ExternalProject_add(spot-ext
            URL http://www.lrde.epita.fr/dload/spot/spot-2.11.3.tar.gz
            URL_HASH SHA512=bb838b5a5eb772f31f1e39389d27f5e6b478713040cd6609a76c534afb491bc477d54f7fa8c7c4800efae74259781fddeff2854f601b685745797faaf7998d20
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            BUILD_COMMAND make -j${DEPENDENCY_JOBS} -C lib COMMAND make -j${DEPENDENCY_JOBS} -C buddy COMMAND make -j${DEPENDENCY_JOBS} -C picosat COMMAND make -j${DEPENDENCY_JOBS} -C ltdl COMMAND make -j${DEPENDENCY_JOBS} -C spot
            CONFIGURE_COMMAND CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} <SOURCE_DIR>/configure --prefix ${EXTERNAL_INSTALL_LOCATION} --disable-python --disable-devel --disable-debug --disable-shared --enable-static --enable-c++17 CFLAGS=${CMAKE_CXX_FLAGS} LDFLAGS=${CMAKE_LINKER_FLAGS} --host=x86_64-w64-mingw32
            INSTALL_COMMAND make -C lib install COMMAND make -C buddy install COMMAND make -C picosat install COMMAND make -C ltdl install COMMAND make -C spot install
            PATCH_COMMAND patch -p1 < ${PROJECT_SOURCE_DIR}/spot-gcc16.patch COMMAND patch -p1 < ${PROJECT_SOURCE_DIR}/spot-windows.patch
        )

    endif ()

    if (WIN32) #If windows 32 or 64
        set(GLPK_CFLAGS "-D __WOE__ -O3 -std=gnu17" )
        ExternalProject_add(glpk-ext
            URL https://ftp.gnu.org/gnu/glpk/glpk-5.0.tar.gz
            URL_HASH SHA512=4e92195fa058c707146f2690f3a38b46c33add948c852f67659ca005a6aa980bbf97be96528b0f8391690facb880ac2126cd60198c6c175e7f3f06cca7e29f9d
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            CONFIGURE_COMMAND CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} <SOURCE_DIR>/configure --enable-shared=no --prefix=${EXTERNAL_INSTALL_LOCATION} CFLAGS=${GLPK_CFLAGS} --host=x86_64-w64-mingw32
            BUILD_COMMAND make -j${DEPENDENCY_JOBS}
            INSTALL_COMMAND make install
        )

    else()
        set(GLPK_CFLAGS "-O3 -std=gnu17" )
        ExternalProject_add(glpk-ext
            URL https://ftp.gnu.org/gnu/glpk/glpk-5.0.tar.gz
            URL_HASH SHA512=4e92195fa058c707146f2690f3a38b46c33add948c852f67659ca005a6aa980bbf97be96528b0f8391690facb880ac2126cd60198c6c175e7f3f06cca7e29f9d
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            CONFIGURE_COMMAND CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} <SOURCE_DIR>/configure --enable-shared=no --prefix=${EXTERNAL_INSTALL_LOCATION} CFLAGS=${GLPK_CFLAGS}
            BUILD_COMMAND make -j${DEPENDENCY_JOBS}
            INSTALL_COMMAND make install
        )

    endif()

    link_directories(${EXTERNAL_INSTALL_LOCATION}/lib)
    link_directories(${EXTERNAL_INSTALL_LOCATION}/lib64)
    include_directories(${EXTERNAL_INSTALL_LOCATION}/include)
endif (VERIFYPN_GetDependencies)

# Set Macros
if (VERIFYPN_MC_Simplification)
    add_compile_definitions(VERIFYPN_MC_Simplification)
endif(VERIFYPN_MC_Simplification)
add_compile_definitions(VERIFYPN_VERSION=\"${VERIFYPN_VERSION}\")

# Source
include_directories(include)
add_subdirectory(${CMAKE_SOURCE_DIR}/src/)

# Testing
if (VERIFYPN_TEST)
    enable_testing()
    add_subdirectory(${CMAKE_SOURCE_DIR}/boost_tests)
endif()
