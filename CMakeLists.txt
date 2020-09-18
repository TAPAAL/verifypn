cmake_minimum_required(VERSION 3.9)
cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0069 NEW)

set(CMAKE_CXX_STANDARD 17)
if (NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Release)
endif (NOT CMAKE_BUILD_TYPE)

if (CMAKE_BUILD_TYPE MATCHES Release)
    if (NOT APPLE)
    	set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif ()
endif ()
set(CMAKE_POSITION_INDEPENDENT_CODE ON)


set(VERIFYPN_VERSION 3.1.0)
add_compile_definitions(VERIFYPN_VERSION=\"${VERIFYPN_VERSION}\")

project(verifypn VERSION ${VERIFYPN_VERSION} LANGUAGES CXX C)

option(VERIFYPN_Static "Link libraries statically" ON)
option(VERIFYPN_MC_Simplification "Enables multicore simplification, incompatible with static linking" OFF)
option(VERIFYPN_GetDependencies "Fetch external dependencies from web." ON)


if (VERIFYPN_Static)
    set(BUILD_SHARED_LIBS OFF)
    if (VERIFYPN_MC_Simplification)
        message( FATAL_ERROR "Multicore Simplification is not compatible with static linking" )
    endif(VERIFYPN_MC_Simplification)
    if (NOT APPLE)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    else(NOT APPLE)
    	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    endif(NOT APPLE)
else(VERIFYPN_Static)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
endif (VERIFYPN_Static)

if (VERIFYPN_MC_Simplification)
    add_compile_definitions(VERIFYPN_MC_Simplification)
endif(VERIFYPN_MC_Simplification)

if (UNIX AND NOT APPLE)
    set(ARCH_TYPE "linux64")
elseif(APPLE)
    set(ARCH_TYPE "osx64")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=10.8 -m64 ")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mmacosx-version-min=10.8 -m64 ")
else()
    set(ARCH_TYPE "win64")
endif ()

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -pedantic-errors -O3 -DNDEBUG")

set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -O3 -DNDEBUG")
if (VERIFYPN_Static AND NOT APPLE)
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -static")
endif()

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

    if (WIN32) #If windows 32 or 64 
      set(GLPK_CFLAGS "-D __WOE__ -O3" )
    else(WIN32)
      set(GLPK_CFLAGS "-O3" )
    endif(WIN32)
    ExternalProject_add(glpk-ext
        URL https://ftp.gnu.org/gnu/glpk/glpk-4.65.tar.gz
        URL_HASH SHA512=997e8e599ff1718a08c66b86eadd0e01f4644899f1e95920f8ae91d66b4d8361021766b346845f4dcbcfe667b41ab72ea3d377017a0ebf85d7ece091cfd81375
	CONFIGURE_COMMAND CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} <SOURCE_DIR>/configure --enable-shared=no --prefix=${EXTERNAL_INSTALL_LOCATION} CFLAGS=${GLPK_CFLAGS}
	BUILD_COMMAND make
        INSTALL_COMMAND make install
        PATCH_COMMAND patch -p1 < ${PROJECT_SOURCE_DIR}/glpk-warning.patch
    )

    link_directories(${EXTERNAL_INSTALL_LOCATION}/lib)
    link_directories(${EXTERNAL_INSTALL_LOCATION}/lib64)

    # we can now include external libraries
    include_directories(${EXTERNAL_INSTALL_LOCATION}/include)
endif (VERIFYPN_GetDependencies)
include_directories(include)

add_subdirectory(${CMAKE_SOURCE_DIR}/src/)
