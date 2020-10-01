set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(LTL_algorithm)

add_library(LTL ${HEADER_FILES} _LTL.cpp Structures/ProductStateSet.cpp)

target_link_libraries(LTL LTL_algorithm bddx)

