set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(Colored)
add_subdirectory(Structures)
add_subdirectory(Simplification)
add_subdirectory(Reachability)
add_subdirectory(PQL)
add_subdirectory(TAR)

add_library(PetriEngine ${HEADER_FILES} 
PetriNet.cpp
PetriNetBuilder.cpp
Reducer.cpp
ReducingSuccessorGenerator.cpp
STSolver.cpp
SuccessorGenerator.cpp
)

target_link_libraries(PetriEngine PRIVATE Colored Structures Simplification Reachability PQL TAR)
