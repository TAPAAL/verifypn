set(CMAKE_INCLUDE_CURRENT_DIR ON)


add_subdirectory(Algorithm)
add_subdirectory(Simplification)
add_subdirectory(SuccessorGeneration)

add_library(LTL ${HEADER_FILES} LTLMain.cpp)

if (VERIFYPN_Static AND NOT APPLE)
    target_link_libraries(LTL PRIVATE LTL_algorithm LTL_simplification LTLSuccessorGeneration spot bddx)
else()
    target_link_libraries(LTL PRIVATE LTL_algorithm LTL_simplification LTLSuccessorGeneration
                          -Wl,-Bstatic spot bddx -Wl,-Bdynamic)
endif()
