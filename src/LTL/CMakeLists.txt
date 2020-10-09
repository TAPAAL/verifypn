set(CMAKE_INCLUDE_CURRENT_DIR ON)
if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_definitions(PRINTF_DEBUG)
endif()

add_subdirectory(LTL_algorithm)

add_library(LTL ${HEADER_FILES} _LTL.cpp)

target_link_libraries(LTL LTL_algorithm bddx)

