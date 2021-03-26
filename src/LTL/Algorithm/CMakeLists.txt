set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(LTL_algorithm ${HEADER_FILES}
        NestedDepthFirstSearch.cpp LTLToBuchi.cpp TarjanModelChecker.cpp RandomNDFS.cpp ModelChecker.cpp ../SuccessorGeneration/ResumingSuccessorGenerator.cpp)

target_link_libraries(LTL_algorithm PetriEngine)
add_dependencies(LTL_algorithm ptrie-ext spot-ext)

