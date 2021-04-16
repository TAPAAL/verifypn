set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(Colored ColoredPetriNetBuilder.cpp
Colors.cpp
Multiset.cpp
BindingGenerator.cpp
PartitionBuilder.cpp
EquivalenceClass.cpp
GuardRestrictor.cpp)
add_dependencies(Colored rapidxml-ext ptrie-ext glpk-ext)
