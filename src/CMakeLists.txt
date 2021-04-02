set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(CTL)
add_subdirectory(LTL)
add_subdirectory(PetriParse)
add_subdirectory(PetriEngine)

add_executable(verifypn-${ARCH_TYPE} VerifyPN.cpp)
add_dependencies(verifypn-${ARCH_TYPE} glpk-ext spot-ext)

target_link_libraries(verifypn-${ARCH_TYPE} PRIVATE CTL LTL PetriEngine PetriParse)

# add_executable(parse-benchmark-asctl ParseOnly.cpp)

 #target_link_libraries(parse-benchmark-asctl PetriEngine PetriParse)

if (VERIFYPN_MC_Simplification)
    target_link_libraries(verifypn-${ARCH_TYPE} PUBLIC pthread)
endif(VERIFYPN_MC_Simplification)

if (APPLE OR NOT VERIFYPN_Static)
    target_link_libraries(verifypn-${ARCH_TYPE} PRIVATE -static-libgcc -static-libstdc++)
endif()

