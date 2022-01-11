
add_library(Synthesis ${HEADER_FILES} SimpleSynthesis.cpp)
target_link_libraries(Synthesis PetriEngine)