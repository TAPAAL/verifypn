set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(Algorithm)
add_subdirectory(DependencyGraph)
add_subdirectory(PetriNets)
add_subdirectory(SearchStrategy)

add_library(CTL ${HEADER_FILES}
CTLEngine.cpp CTLResult.cpp)
add_dependencies(PetriNets ptrie-ext spot-ext)

target_link_libraries(CTL Algorithm DependencyGraph PetriNets SearchStrategy)

