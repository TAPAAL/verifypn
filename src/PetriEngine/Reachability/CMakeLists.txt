set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(Reachability ReachabilitySearch.cpp  ResultPrinter.cpp  TARReachability.cpp)
target_link_libraries(Reachability Structures)
