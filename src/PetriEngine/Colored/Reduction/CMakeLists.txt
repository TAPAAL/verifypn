set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(ColoredReduction ${HEADER_FILES}
        ColoredReducer.cpp
        RedRulePreAgglomeration.cpp
        RedRuleParallelTransitions.cpp
        RedRuleParallelPlaces.cpp)

target_link_libraries(ColoredReduction Colored)