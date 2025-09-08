
add_library(Synthesis ${HEADER_FILES} SimpleSynthesis.cpp GameSuccessorGenerator.cpp GamePORSuccessorGenerator.cpp GameStubbornSet.cpp)
target_link_libraries(Synthesis PetriEngine)
add_dependencies(PetriNets glpk-ext ptrie-ext)