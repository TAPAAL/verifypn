set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(CTL)
add_subdirectory(LTL)
add_subdirectory(PetriParse)
add_subdirectory(PetriEngine)

add_executable(verifypn-${ARCH_TYPE} VerifyPN.cpp)
add_dependencies(verifypn-${ARCH_TYPE} glpk-ext)

target_link_libraries(verifypn-${ARCH_TYPE} PRIVATE CTL PetriEngine PetriParse glpk)

add_executable(verifyltl-${ARCH_TYPE} LTLMain.cpp)
add_dependencies(verifyltl-${ARCH_TYPE} glpk-ext spot-ext)

target_link_libraries(verifyltl-${ARCH_TYPE} CTL LTL PetriEngine PetriParse glpk spot)

# add_executable(parse-benchmark-asctl ParseOnly.cpp)

 #target_link_libraries(parse-benchmark-asctl PetriEngine PetriParse)

if (VERIFYPN_MC_Simplification)
    target_link_libraries(verifypn-${ARCH_TYPE} PUBLIC pthread)
endif(VERIFYPN_MC_Simplification)
