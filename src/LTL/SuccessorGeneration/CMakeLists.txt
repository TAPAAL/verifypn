set(CMAKE_INCLUDE_CURRENT_DIR ON)

flex_target(heur_lexer "${CMAKE_CURRENT_SOURCE_DIR}/HeuristicLexer.l"
        "${CMAKE_CURRENT_SOURCE_DIR}/HeuristicLexer.lexer.cpp")

bison_target(heur_parser "${CMAKE_CURRENT_SOURCE_DIR}/HeuristicParser.y"
        "${CMAKE_CURRENT_SOURCE_DIR}/HeuristicParser.parser.cpp")

add_library(LTLSuccessorGeneration ${BISON_heur_parser_OUTPUTS} ${FLEX_heur_lexer_OUTPUTS} ${HEADER_FILES} ResumingSuccessorGenerator.cpp AutomatonHeuristic.cpp)

target_link_libraries(LTLSuccessorGeneration bddx spot)

