set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(LTLStubborn ${HEADER_FILES} EvalAndSetVisitor.cpp VisibleTransitionVisitor.cpp VisibleLTLStubbornSet.cpp InterestingLTLStubbornSet.cpp)
add_dependencies(LTLStubborn PetriEngine)
