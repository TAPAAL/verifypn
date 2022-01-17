set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(CTL)
add_subdirectory(LTL)
add_subdirectory(PetriParse)
add_subdirectory(PetriEngine)

add_library(verifypn VerifyPN.cpp)
target_link_libraries(verifypn PUBLIC CTL LTL PetriEngine PetriParse)

add_dependencies(verifypn glpk-ext spot-ext)

add_executable(verifypn-${ARCH_TYPE} main.cpp)
target_link_libraries(verifypn-${ARCH_TYPE} PRIVATE verifypn)

if (VERIFYPN_MC_Simplification)
    target_link_libraries(verifypn-${ARCH_TYPE} PUBLIC pthread)
endif(VERIFYPN_MC_Simplification)

if (APPLE OR NOT VERIFYPN_Static)
    target_link_libraries(verifypn-${ARCH_TYPE} PUBLIC -static-libgcc -static-libstdc++)
elseif (NOT APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()



