set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(LTLStubborn LTLStubbornSet.cpp EvalAndSetVisitor.cpp)
add_dependencies(LTLStubborn PetriEngine)
