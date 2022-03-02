set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(ColoredReduction ${HEADER_FILES}
        ColoredReducer.cpp
        RedRuleIdentity.cpp)
