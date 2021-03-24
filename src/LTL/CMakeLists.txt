set(CMAKE_INCLUDE_CURRENT_DIR ON)


add_subdirectory(Algorithm)
add_subdirectory(Simplification)

add_library(LTL ${HEADER_FILES} _LTL.cpp)

if (VERIFYPN_Static AND NOT APPLE)
    target_link_libraries(LTL PRIVATE LTL_algorithm LTL_simplification spot bddx)
else()
    target_link_libraries(LTL PRIVATE LTL_algorithm LTL_simplification
                          -Wl,-Bstatic spot bddx -Wl,-Bdynamic)
endif()
