set(CMAKE_INCLUDE_CURRENT_DIR ON)


add_subdirectory(Algorithm)
add_subdirectory(Simplification)

add_library(LTL ${HEADER_FILES} _LTL.cpp)

target_link_libraries(LTL PRIVATE LTL_algorithm LTL_simplification
                          -Wl,-Bstatic spot bddx -Wl,-Bdynamic)
