set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(PetriParse ${HEADER_FILES} AbstractPetriNetBuilder.cpp PNMLParser.cpp QueryBinaryParser.cpp QueryXMLParser.cpp)
add_dependencies(PetriParse PetriEngine rapidxml-ext)