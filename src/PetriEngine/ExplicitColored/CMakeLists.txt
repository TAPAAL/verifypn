set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(ExplicitColored 
    GuardExpression.cpp
    ArcExpression.cpp
)

target_link_libraries(ExplicitColored Colored)
