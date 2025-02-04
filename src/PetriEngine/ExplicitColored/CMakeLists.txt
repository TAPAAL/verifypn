set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(ExplicitColored
    ArcCompiler.cpp
    ColoredPetriNetBuilder.cpp
    ColoredSuccessorGenerator.cpp
    Algorithms/NaiveWorklist.cpp
    ColoredResultPrinter.cpp
    GuardCompiler.cpp
    GammaQueryCompiler.cpp
)

target_link_libraries(ExplicitColored Colored)
