set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(LTL ${HEADER_FILES}
        LTLToBuchi.cpp)

target_link_libraries(LTL bddx)

