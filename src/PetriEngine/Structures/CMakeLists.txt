set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(Structures AlignedEncoder.cpp  binarywrapper.cpp  Queue.cpp)
add_dependencies(Structures ptrie-ext glpk-ext)
