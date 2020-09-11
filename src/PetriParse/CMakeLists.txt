set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(PetriParse ${HEADER_FILES} PNMLParser.cpp QueryBinaryParser.cpp QueryXMLParser.cpp)
add_dependencies(PetriParse PetriEngine rapidxml-ext)

