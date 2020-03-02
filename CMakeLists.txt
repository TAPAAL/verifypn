cmake_minimum_required(VERSION 3.9)
cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0069 NEW)

set(CMAKE_CXX_STANDARD 17)
if (NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Release)
endif (NOT CMAKE_BUILD_TYPE)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project(verifypn VERSION 3.0.1 LANGUAGES CXX C)

option(VERIFYPN_Static "Link libraries statically" OFF)
option(VERIFYPN_MCC "Compile MCC-version, incompatible with static linking" ON)
option(VERIFYPN_GetDependencies "Fetch external dependencies from web." ON)

if (VERIFYPN_Static)
    set(BUILD_SHARED_LIBS OFF)
    set(Boost_USE_STATIC_LIBS ON)
else()
    if (NOT APPLE)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    endif()
endif (VERIFYPN_Static)

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
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -g -O1")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if (VERIFYPN_GetDependencies)
    # setup for external imports
    include(ExternalProject)
    set(EXTERNAL_INSTALL_LOCATION ${CMAKE_BINARY_DIR}/external)

    #ExternalProject_add(ptrie-ext
    #    URL https://github.com/petergjoel/ptrie/archive/1.0.3.zip
    #    URL_HASH SHA512=3f8285a0a7008dd0a8850753b748ad2d076df3f02574005f6580fa750f9a4aaa808e297a60c2b91d705e2ca63b1eb83ecd4e4504ae420fb2fd94d8f10ea42dba
    #    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_LOCATION} -DPTRIE_BuildTest=OFF -DCMAKE_BUILD_TYPE=Release
    #)
    ExternalProject_add(rapidxml-ext
        URL https://downloads.sourceforge.net/project/rapidxml/rapidxml/rapidxml%201.13/rapidxml-1.13.zip
        URL_HASH SHA512=6c10583e6631ccdb0217d0a5381172cb4c1046226de6ef1acf398d85e81d145228e14c3016aefcd7b70a1db8631505b048d8b4f5d4b0dbf1811d2482eefdd265
        BUILD_COMMAND ""
        CONFIGURE_COMMAND mkdir -p ${CMAKE_BINARY_DIR}/external/include
        INSTALL_COMMAND cd ../rapidxml-ext && ${CMAKE_COMMAND} -E copy rapidxml.hpp rapidxml_iterators.hpp rapidxml_print.hpp rapidxml_utils.hpp ${EXTERNAL_INSTALL_LOCATION}/include    
    )

    # we can now include external libraries
    include_directories(${EXTERNAL_INSTALL_LOCATION}/include)
    link_directories(${EXTERNAL_INSTALL_LOCATION}/lib)
endif (VERIFYPN_GetDependencies)
include_directories(include)

add_subdirectory(${CMAKE_SOURCE_DIR}/src/)
