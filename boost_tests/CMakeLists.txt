set (BOOST_USE_STATIC_LIBS OFF)
find_package (Boost COMPONENTS unit_test_framework REQUIRED)
add_definitions (-DBOOST_TEST_DYN_LINK)

include_directories (${TEST_SOURCE_DIR}/include
                     ${Boost_INCLUDE_DIRS}
                     ${unfoldtacpn_SOURCE_DIR}
                     )

add_executable (Boost_Tests_run BinaryPrinterTests.cpp XMLPrinterTests.cpp)

target_link_libraries(Boost_Tests_run PRIVATE CTL LTL PetriEngine PetriParse)

target_compile_definitions(Boost_Tests_run PRIVATE "BOOST_TEST_DYN_LINK=1")

add_test(NAME Boost_Tests_run COMMAND Boost_Tests_run)




add_executable (reachability reachability_test.cpp)
add_executable (games game_test.cpp)

target_link_libraries(reachability PUBLIC ${Boost_LIBRARIES} -Wl,-Bstatic verifypn -Wl,-Bdynamic)
target_link_libraries(games        PUBLIC ${Boost_LIBRARIES} -Wl,-Bstatic verifypn -Wl,-Bdynamic)

add_test(NAME reachability COMMAND reachability)
add_test(NAME games COMMAND games)

set_tests_properties(reachability PROPERTIES
    ENVIRONMENT TEST_FILES=${CMAKE_CURRENT_SOURCE_DIR})
set_tests_properties(games PROPERTIES
    ENVIRONMENT TEST_FILES=${CMAKE_CURRENT_SOURCE_DIR})
