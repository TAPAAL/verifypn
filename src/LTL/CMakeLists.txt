set(CMAKE_INCLUDE_CURRENT_DIR ON)


add_subdirectory(Algorithm)
add_subdirectory(Simplification)

add_library(LTL ${HEADER_FILES} _LTL.cpp)

target_link_libraries(LTL LTL_algorithm bddx spot LTL_simplification)

