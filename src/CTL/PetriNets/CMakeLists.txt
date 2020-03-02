set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(PetriNets OnTheFlyDG.cpp)
target_link_libraries(PetriNets PetriEngine DependencyGraph)
