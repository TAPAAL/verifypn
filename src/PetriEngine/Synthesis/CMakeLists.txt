
add_library(Synthesis ${HEADER_FILES} ReachabilitySynthesis.cpp)
target_link_libraries(Synthesis PetriEngine)