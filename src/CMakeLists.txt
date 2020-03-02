set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(CTL)
add_subdirectory(PetriParse)
add_subdirectory(PetriEngine)

add_executable(verifypn-${ARCH_TYPE} VerifyPN.cpp)
target_link_libraries(verifypn-${ARCH_TYPE} PRIVATE CTL PetriEngine PetriParse)

