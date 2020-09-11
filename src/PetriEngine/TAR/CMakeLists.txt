


set(CMAKE_INCLUDE_CURRENT_DIR ON)


add_library(TAR ${HEADER_FILES} TraceSet.cpp Solver.cpp TARReachability RangeContext.cpp RangeEvalContext.cpp PlaceUseVisitor.cpp)

add_dependencies(TAR glpk-ext)
