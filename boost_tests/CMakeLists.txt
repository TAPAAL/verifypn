set (BOOST_USE_STATIC_LIBS OFF)
find_package (Boost COMPONENTS unit_test_framework REQUIRED)
add_definitions (-DBOOST_TEST_DYN_LINK)

include_directories (${TEST_SOURCE_DIR}/include
                     ${Boost_INCLUDE_DIRS}
                     ${unfoldtacpn_SOURCE_DIR}
                     )

add_executable (BinaryPrinterTests BinaryPrinterTests.cpp)
add_executable (XMLPrinterTests XMLPrinterTests.cpp)
add_executable (PQLParserTests PQLParserTests.cpp)
add_executable (PredicateCheckerTests PredicateCheckerTests.cpp)
add_executable (reachability reachability_test.cpp)
add_executable (ltl ltl_test.cpp)
add_executable (games game_test.cpp)
add_executable (color color_test.cpp)
add_executable (reduction reduction.cpp)

target_link_libraries(BinaryPrinterTests PUBLIC ${Boost_LIBRARIES} -Wl,-Bstatic verifypn -Wl,-Bdynamic)
target_link_libraries(XMLPrinterTests    PUBLIC ${Boost_LIBRARIES} -Wl,-Bstatic verifypn -Wl,-Bdynamic)
target_link_libraries(PQLParserTests     PUBLIC ${Boost_LIBRARIES} -Wl,-Bstatic verifypn -Wl,-Bdynamic)
target_link_libraries(PredicateCheckerTests     PUBLIC ${Boost_LIBRARIES} -Wl,-Bstatic verifypn -Wl,-Bdynamic)
target_link_libraries(reachability PUBLIC ${Boost_LIBRARIES} -Wl,-Bstatic verifypn -Wl,-Bdynamic)
target_link_libraries(ltl PUBLIC ${Boost_LIBRARIES} -Wl,-Bstatic verifypn -Wl,-Bdynamic)
target_link_libraries(games        PUBLIC ${Boost_LIBRARIES} -Wl,-Bstatic verifypn -Wl,-Bdynamic)
target_link_libraries(color        PUBLIC ${Boost_LIBRARIES} -Wl,-Bstatic verifypn -Wl,-Bdynamic)
target_link_libraries(reduction        PUBLIC ${Boost_LIBRARIES} -Wl,-Bstatic verifypn -Wl,-Bdynamic)

add_test(NAME BinaryPrinterTests COMMAND BinaryPrinterTests)
add_test(NAME XMLPrinterTests COMMAND XMLPrinterTests)
add_test(NAME PQLParserTests COMMAND PQLParserTests)
add_test(NAME PredicateCheckerTests COMMAND PredicateCheckerTests)
add_test(NAME reachability COMMAND reachability)
add_test(NAME ltl COMMAND ltl)
add_test(NAME games COMMAND games)
add_test(NAME color COMMAND color)
add_test(NAME reduction COMMAND reduction)

set_tests_properties(reachability PROPERTIES
    ENVIRONMENT TEST_FILES=${CMAKE_CURRENT_SOURCE_DIR})
set_tests_properties(ltl PROPERTIES
    ENVIRONMENT TEST_FILES=${CMAKE_CURRENT_SOURCE_DIR})
set_tests_properties(games PROPERTIES
    ENVIRONMENT TEST_FILES=${CMAKE_CURRENT_SOURCE_DIR})
set_tests_properties(color PROPERTIES
    ENVIRONMENT TEST_FILES=${CMAKE_CURRENT_SOURCE_DIR})
set_tests_properties(reduction PROPERTIES
    ENVIRONMENT TEST_FILES=${CMAKE_CURRENT_SOURCE_DIR})
set_tests_properties(PredicateCheckerTests PROPERTIES
    ENVIRONMENT TEST_FILES=${CMAKE_CURRENT_SOURCE_DIR})
set_tests_properties(BinaryPrinterTests PROPERTIES
    ENVIRONMENT TEST_FILES=${CMAKE_CURRENT_SOURCE_DIR})
set_tests_properties(XMLPrinterTests PROPERTIES
    ENVIRONMENT TEST_FILES=${CMAKE_CURRENT_SOURCE_DIR})
set_tests_properties(PQLParserTests PROPERTIES
    ENVIRONMENT TEST_FILES=${CMAKE_CURRENT_SOURCE_DIR})