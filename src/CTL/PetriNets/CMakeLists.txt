set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(PetriNets OnTheFlyDG.cpp)
add_dependencies(PetriNets ptrie-ext)
target_link_libraries(PetriNets PetriEngine DependencyGraph)
