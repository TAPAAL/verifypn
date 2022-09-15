set (BOOST_USE_STATIC_LIBS OFF)
find_package (Boost COMPONENTS unit_test_framework REQUIRED)


#add_executable (Boost_Tests_run BinaryPrinterTests.cpp XMLPrinterTests.cpp) // disabled for now

#target_link_libraries(Boost_Tests_run PRIVATE CTL LTL PetriEngine PetriParse)

#target_include_directories (Boost_Tests_run PRIVATE ${Boost_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/include)
#target_compile_definitions(Boost_Tests_run PRIVATE "BOOST_TEST_DYN_LINK=1")
