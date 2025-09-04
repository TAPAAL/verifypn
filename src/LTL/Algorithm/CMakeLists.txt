set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(LTL_algorithm ${HEADER_FILES}
        NestedDepthFirstSearch.cpp LTLToBuchi.cpp TarjanModelChecker.cpp)

target_link_libraries(LTL_algorithm PetriEngine LTLStubborn)
add_dependencies(LTL_algorithm glpk-ext ptrie-ext spot-ext)

