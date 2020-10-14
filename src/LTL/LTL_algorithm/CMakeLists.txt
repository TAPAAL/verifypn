set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(LTL_algorithm ${HEADER_FILES}
        NestedDepthFirstSearch.cpp ModelChecker.cpp ../ProductSuccessorGenerator.cpp LTLToBuchi.cpp)

target_link_libraries(LTL_algorithm PetriEngine spot)
add_dependencies(LTL_algorithm ptrie-ext spot-ext)

