set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(LTLStructures ${HEADER_FILES} GuardInfo.cpp)
add_dependencies(LTLStructures PetriEngine)
