set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(LTL_simplification ${HEADER_FILES}
        SpotToPQL.cpp)

add_dependencies(LTL_simplification glpk-ext spot-ext)
target_link_libraries(LTL_simplification LTL_algorithm PetriEngine)

