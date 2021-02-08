set(CMAKE_INCLUDE_CURRENT_DIR ON)


add_subdirectory(Algorithm)
add_subdirectory(Simplification)
add_subdirectory(Stubborn)

add_library(LTL ${HEADER_FILES} LTLMain.cpp)

target_link_libraries(LTL LTL_algorithm bddx spot LTLStubborn LTL_simplification)

