set(CMAKE_INCLUDE_CURRENT_DIR ON)


add_subdirectory(Algorithm)
add_subdirectory(Simplification)

add_subdirectory(Stubborn)
add_subdirectory(SuccessorGeneration)

add_library(LTL ${HEADER_FILES} LTLSearch.cpp)

if (VERIFYPN_Static OR APPLE)
    target_link_libraries(LTL PUBLIC LTL_algorithm LTLStubborn LTL_simplification LTLSuccessorGeneration spot bddx)
else()
    target_link_libraries(LTL PUBLIC LTL_algorithm LTLStubborn LTL_simplification LTLSuccessorGeneration
                          -Wl,-Bstatic spot bddx -Wl,-Bdynamic)
endif()
