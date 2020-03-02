set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(Algorithm)
add_subdirectory(DependencyGraph)
add_subdirectory(PetriNets)
add_subdirectory(SearchStrategy)

add_library(CTL ${HEADER_FILES}
CTLEngine.cpp)

target_link_libraries(CTL Algorithm DependencyGraph PetriNets SearchStrategy)

