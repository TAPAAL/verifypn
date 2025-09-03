set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(LTLStubborn LTLEvalAndSetVisitor.cpp VisibleTransitionVisitor.cpp VisibleLTLStubbornSet.cpp
                        AutomatonStubbornSet.cpp SafeAutStubbornSet.cpp)
target_link_libraries(LTLStubborn PetriEngine)
add_dependencies(LTLStubborn glpk-ext spot-ext)