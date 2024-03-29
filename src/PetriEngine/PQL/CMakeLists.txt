set(CMAKE_INCLUDE_CURRENT_DIR ON)

flex_target(pql_lexer "${CMAKE_CURRENT_SOURCE_DIR}/PQLQueryTokens.l"
                      "${CMAKE_CURRENT_SOURCE_DIR}/PQLQueryTokens.lexer.cpp")

bison_target(pql_parser "${CMAKE_CURRENT_SOURCE_DIR}/PQLQueryParser.y"
                        "${CMAKE_CURRENT_SOURCE_DIR}/PQLQueryParser.parser.cpp")

add_flex_bison_dependency(pql_lexer pql_parser)

add_library(PQL ${BISON_pql_parser_OUTPUTS} ${FLEX_pql_lexer_OUTPUTS} Expressions.cpp PQL.cpp
 Contexts.cpp QueryPrinter.cpp CTLVisitor.cpp XMLPrinter.cpp BinaryPrinter.cpp
    Simplifier.cpp PushNegation.cpp FormulaSize.cpp PrepareForReachability.cpp PredicateCheckers.cpp
    PlaceUseVisitor.cpp Analyze.cpp Evaluation.cpp ColoredUseVisitor.cpp PotencyVisitor.cpp)

add_dependencies(PQL glpk-ext)
target_link_libraries(PQL Simplification Reachability glpk PetriEngine)

