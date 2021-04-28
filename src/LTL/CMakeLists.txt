set(CMAKE_INCLUDE_CURRENT_DIR ON)


add_subdirectory(Algorithm)
add_subdirectory(Structures)
add_subdirectory(Stubborn)
add_subdirectory(SuccessorGeneration)
add_subdirectory(Simplification)

add_library(LTL ${HEADER_FILES} LTLMain.cpp)

if (VERIFYPN_Static OR APPLE)
    target_link_libraries(LTL PRIVATE LTL_algorithm LTL_simplification LTLSuccessorGeneration LTLStubborn spot bddx)
else()
    target_link_libraries(LTL PRIVATE LTL_algorithm LTL_simplification LTLSuccessorGeneration LTLStubborn
                              -Wl,-Bstatic spot bddx -Wl,-Bdynamic)
endif()
