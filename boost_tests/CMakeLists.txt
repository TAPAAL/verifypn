set (BOOST_USE_STATIC_LIBS OFF)
find_package (Boost COMPONENTS unit_test_framework REQUIRED)
add_definitions (-DBOOST_TEST_DYN_LINK)

add_executable (Boost_Tests_run BinaryPrinterTests.cpp XMLPrinterTests.cpp)

target_link_libraries(Boost_Tests_run PRIVATE CTL LTL PetriEngine PetriParse)

target_include_directories (Boost_Tests_run PRIVATE ${Boost_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/include)
target_compile_definitions(Boost_Tests_run PRIVATE "BOOST_TEST_DYN_LINK=1")

add_test(NAME Boost_Tests_run COMMAND Boost_Tests_run)

add_executable (reachability reachability_test.cpp)
target_link_libraries(reachability ${Boost_LIBRARIES} verifypn)
add_test(NAME reachability COMMAND reachability)

set_tests_properties(reachability PROPERTIES
    ENVIRONMENT TEST_FILES=${CMAKE_CURRENT_SOURCE_DIR})
