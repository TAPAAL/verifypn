set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(ExplicitColored
    ArcCompiler.cpp
    ColoredPetriNetBuilder.cpp
    ColoredSuccessorGenerator.cpp
    Algorithms/ExplicitWorklist.cpp
    ColoredResultPrinter.cpp
    GuardCompiler.cpp
    GammaQueryCompiler.cpp
    ColoredPetriNet.cpp
)

target_link_libraries(ExplicitColored Colored)
