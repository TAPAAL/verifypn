set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(ExplicitColored
    ArcCompiler.cpp
        ExplicitColoredPetriNetBuilder.cpp
    SuccessorGenerator/ColoredSuccessorGenerator.cpp
    Algorithms/ExplicitWorklist.cpp
    Algorithms/FireabilitySearch.cpp
    ColoredResultPrinter.cpp
    ExpressionCompilers/GuardCompiler.cpp
    ExpressionCompilers/GammaQueryCompiler.cpp
        IgnorantFireabilityVisitor.cpp
    ColoredPetriNet.cpp
    ColorIgnorantPetriNetBuilder.cpp
    ExplicitColoredModelChecker.cpp
)

target_link_libraries(ExplicitColored Colored)
