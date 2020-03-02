set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(Reachability ReachabilitySearch.cpp  ResultPrinter.cpp  TARReachability.cpp)
add_dependencies(Reachability ptrie-ext)
target_link_libraries(Reachability Structures)
