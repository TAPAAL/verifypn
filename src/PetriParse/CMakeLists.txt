set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(PetriParse ${HEADER_FILES} AbstractPetriNetBuilder.cpp PNMLParser.cpp QueryBinaryParser.cpp QueryXMLParser.cpp)
target_link_libraries(PetriParse PetriEngine)
add_dependencies(PetriParse rapidxml-ext)
