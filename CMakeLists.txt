cmake_minimum_required(VERSION 3.9)
cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0069 NEW)

set(CMAKE_CXX_STANDARD 17)
if (NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Release)
endif (NOT CMAKE_BUILD_TYPE)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

set(VERIFYPN_VERSION 3.0.1)
add_compile_definitions(VERIFYPN_VERSION=\"${VERIFYPN_VERSION}\")

project(verifypn VERSION ${VERIFYPN_VERSION} LANGUAGES CXX C)

option(VERIFYPN_Static "Link libraries statically" OFF)
option(VERIFYPN_TAR "Enables TAR, incompatible with static linking" OFF)
option(VERIFYPN_MC_Simplification "Enables multicore simplification, incompatible with static linking" ON)
option(VERIFYPN_GetDependencies "Fetch external dependencies from web." ON)


if (VERIFYPN_Static)
    set(BUILD_SHARED_LIBS OFF)
    if (VERIFYPN_TAR)
        message( FATAL_ERROR "TAR is not compatible with static linking" )
    endif()
    if (VERIFYPN_MC_Simplification)
        message( FATAL_ERROR "Multicore Simplification is not compatible with static linking" )
    endif()
else()
    if (NOT APPLE)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    endif()
endif (VERIFYPN_Static)

if (VERIFYPN_TAR)
    add_compile_definitions(VERIFYPN_TAR)
endif(VERIFYPN_TAR)
if (VERIFYPN_MC_Simplification)
    add_compile_definitions(VERIFYPN_MC_Simplification)
endif(VERIFYPN_MC_Simplification)

if (UNIX AND NOT APPLE)
    set(LPLIB "${CMAKE_SOURCE_DIR}/include/lpsolve/liblpsolve55-linux64.a")
    set(ARCH_TYPE "linux64")
elseif(APPLE)
    set(LPLIB "${CMAKE_SOURCE_DIR}/include/lpsolve/liblpsolve55-osx64.a")
    set(ARCH_TYPE "osx64")
else()
    set(LPLIB "${CMAKE_SOURCE_DIR}/include/lpsolve/liblpsolve55-win64.dll")
    set(ARCH_TYPE "win64")
endif ()

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -pedantic-errors -O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS_DEBUG} -g -O1")

set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -O3 -DNDEBUG")
if (VERIFYPN_Static AND NOT APPLE)
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -static")
endif(VERIFYPN_Static)
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -g -O1")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if (VERIFYPN_GetDependencies)
    # setup for external imports
    include(ExternalProject)
    set(EXTERNAL_INSTALL_LOCATION ${CMAKE_BINARY_DIR}/external)

    ExternalProject_add(ptrie-ext
        GIT_REPOSITORY https://github.com/petergjoel/ptrie
        GIT_TAG 230b3640bfbe2ed5befdafbaf17bd3804231b50f
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_LOCATION} -DPTRIE_BuildTest=OFF -DCMAKE_BUILD_TYPE=Release
    )
    ExternalProject_add(rapidxml-ext
        URL https://downloads.sourceforge.net/project/rapidxml/rapidxml/rapidxml%201.13/rapidxml-1.13.zip
        URL_HASH SHA512=6c10583e6631ccdb0217d0a5381172cb4c1046226de6ef1acf398d85e81d145228e14c3016aefcd7b70a1db8631505b048d8b4f5d4b0dbf1811d2482eefdd265
        BUILD_COMMAND ""
        CONFIGURE_COMMAND mkdir -p ${CMAKE_BINARY_DIR}/external/include
        INSTALL_COMMAND cd ../rapidxml-ext && ${CMAKE_COMMAND} -E copy rapidxml.hpp rapidxml_iterators.hpp rapidxml_print.hpp rapidxml_utils.hpp ${EXTERNAL_INSTALL_LOCATION}/include    
    )

    if (VERIFYPN_TAR)
        ExternalProject_add(z3-ext
            URL https://github.com/Z3Prover/z3/releases/download/z3-4.7.1/z3-4.7.1.zip
            URL_HASH SHA512=c1d1bd60ef7e41a6a5abc2b5e6a548c5e32573b76d958c244457db636b54adbd322b5d42a3bf85a08759c69860b9ce32e924e9c9135ef396cdc75df3944c516e
            CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_LOCATION} -DUSE_OPENMP=FALSE -DUSE_LIB_GMP=TRUE -DENABLE_EXAMPLE_TARGETS=FALSE -DLINK_TIME_OPTIMIZATION=TRUE -DBUILD_LIBZ3_SHARED=TRUE
        )
        link_directories(${EXTERNAL_INSTALL_LOCATION}/lib)
        link_directories(${EXTERNAL_INSTALL_LOCATION}/lib64)
    endif(VERIFYPN_TAR)

    # we can now include external libraries
    include_directories(${EXTERNAL_INSTALL_LOCATION}/include)
endif (VERIFYPN_GetDependencies)
include_directories(include)

add_subdirectory(${CMAKE_SOURCE_DIR}/src/)
