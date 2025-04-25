set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(Reduction)

add_library(Colored ColoredPetriNetBuilder.cpp
Colors.cpp
Multiset.cpp
BindingGenerator.cpp
PartitionBuilder.cpp
EquivalenceClass.cpp
GuardRestrictor.cpp
IntervalGenerator.cpp
EquivalenceVec.cpp
CExprToString.cpp
EvaluationVisitor.cpp
ForwardFixedPoint.cpp
VariableSymmetry.cpp
Unfolder.cpp
PnmlWriter.cpp
PnmlWriterColorExprVisitor.cpp
VarMultiset.cpp
ArcVarMultisetVisitor.cpp
CloningVisitor.cpp
VarReplaceVisitor.cpp)



target_link_libraries(Colored ColoredReduction)
add_dependencies(Colored rapidxml-ext ptrie-ext glpk-ext)
