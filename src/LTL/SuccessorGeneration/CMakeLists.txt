set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(LTLSuccessorGeneration ${HEADER_FILES} ResumingSuccessorGenerator.cpp CompoundGenerator.cpp AutomatonHeuristic.cpp)

target_link_libraries(LTLSuccessorGeneration bddx)
add_dependencies(LTLSuccessorGeneration glpk-ext spot-ext)

