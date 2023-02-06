


set(CMAKE_INCLUDE_CURRENT_DIR ON)


add_library(TAR ${HEADER_FILES} TraceSet.cpp Solver.cpp TARReachability.cpp RangeContext.cpp RangeEvalContext.cpp)
target_link_libraries(TAR PQL)
add_dependencies(TAR glpk-ext ptrie-ext)
