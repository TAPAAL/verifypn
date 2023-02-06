set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(ColoredReduction ${HEADER_FILES}
        ColoredReducer.cpp
        RedRulePreAgglomeration.cpp
        RedRuleParallelTransitions.cpp
        RedRuleParallelPlaces.cpp
        RedRuleRelevance.cpp
        RedRuleDeadTransitions.cpp
        RedRuleRedundantPlaces.cpp
        RedRulePreemptiveFiring.cpp)

target_link_libraries(ColoredReduction Colored)