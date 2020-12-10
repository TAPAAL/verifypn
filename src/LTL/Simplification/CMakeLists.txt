set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(LTL_simplification ${HEADER_FILES}
        SpotToPQL.cpp)

add_dependencies(LTL_simplification spot-ext)
target_link_libraries(LTL_simplification spot PetriEngine)

