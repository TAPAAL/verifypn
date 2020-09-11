set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(FLEX 2.6.4 REQUIRED)
find_package(BISON 3.0.5 REQUIRED)

flex_target(pql_lexer "${CMAKE_CURRENT_SOURCE_DIR}/PQLQueryTokens.l"
                      "${CMAKE_CURRENT_SOURCE_DIR}/PQLQueryTokens.lexer.cpp")

bison_target(pql_parser "${CMAKE_CURRENT_SOURCE_DIR}/PQLQueryParser.y"
                        "${CMAKE_CURRENT_SOURCE_DIR}/PQLQueryParser.parser.cpp")

add_flex_bison_dependency(pql_lexer pql_parser)

add_library(PQL ${BISON_pql_parser_OUTPUTS} ${FLEX_pql_lexer_OUTPUTS} Expressions.cpp PQL.cpp Contexts.cpp)
add_dependencies(PQL glpk-ext)
target_link_libraries(PQL Simplification Reachability glpk)

