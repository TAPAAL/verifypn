set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(LTL_algorithm ${HEADER_FILES}
        NestedDepthFirstSearch.cpp LTLToBuchi.cpp TarjanModelChecker.cpp ProductSuccessorGenerator.cpp ProductPrinter.cpp RandomNDFS.cpp VisibleStubbornTarjanModelChecker.cpp AutomataStubbornTarjan.cpp InterestingStubbornTarjanModelChecker.cpp ResumingStubbornTarjan.cpp)

target_link_libraries(LTL_algorithm PetriEngine spot bddx)
add_dependencies(LTL_algorithm ptrie-ext spot-ext)

