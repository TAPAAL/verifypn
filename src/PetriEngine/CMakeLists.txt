set(CMAKE_INCLUDE_CURRENT_DIR ON)
#everything in this folder is too co-dependent to become a singular library

add_subdirectory(Colored)
add_subdirectory(Structures)
add_subdirectory(Simplification)
add_subdirectory(Reachability)
add_subdirectory(PQL)

add_library(PetriEngine ${HEADER_FILES} 
PetriNet.cpp
PetriNetBuilder.cpp
Reducer.cpp
ReducingSuccessorGenerator.cpp
STSolver.cpp
SuccessorGenerator.cpp
)

target_link_libraries(PetriEngine PRIVATE Colored Structures Simplification Reachability PQL ${LPLIB})
