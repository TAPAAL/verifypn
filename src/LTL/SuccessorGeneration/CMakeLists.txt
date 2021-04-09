set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(LTLSuccessorGeneration ${HEADER_FILES} ResumingSuccessorGenerator.cpp AutomatonHeuristic.cpp)

target_link_libraries(LTLSuccessorGeneration bddx spot)

