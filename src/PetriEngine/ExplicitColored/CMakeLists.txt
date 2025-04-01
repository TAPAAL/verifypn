set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(ExplicitColored
    ArcCompiler.cpp
    ColoredPetriNetBuilder.cpp
    SuccessorGenerator/ColoredSuccessorGenerator.cpp
    Algorithms/ExplicitWorklist.cpp
    ColoredResultPrinter.cpp
    ExpressionCompilers/GuardCompiler.cpp
    ExpressionCompilers/GammaQueryCompiler.cpp
    fireabilityEvaluate.cpp
    ColoredPetriNet.cpp
    ColorIgnorantPetriNetBuilder.cpp
    ExplicitColoredModelChecker.cpp
)

target_link_libraries(ExplicitColored Colored)
