set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(Stubborn ReachabilityStubbornSet.cpp StubbornSet.cpp InterestingTransitionVisitor.cpp)
target_link_libraries(Stubborn PetriEngine)
add_dependencies(Stubborn glpk-ext)
