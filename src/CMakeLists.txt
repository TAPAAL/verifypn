set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(CTL)
add_subdirectory(LTL)
add_subdirectory(PetriParse)
add_subdirectory(PetriEngine)

add_executable(verifypn-${ARCH_TYPE} VerifyPN.cpp PetriEngine/Stubborn/StubbornSet.cpp ../include/PetriEngine/Stubborn/StubbornSet.h PetriEngine/Stubborn/ReachabilityStubbornSet.cpp ../include/PetriEngine/Stubborn/ReachabilityStubbornSet.h PetriEngine/Stubborn/InterestingTransitionVisitor.cpp ../include/PetriEngine/Stubborn/InterestingTransitionVisitor.h LTL/Stubborn/LTLStubbornSet.cpp ../include/LTL/Stubborn/LTLStubbornSet.h LTL/Stubborn/VisibleTransitionVisitor.cpp ../include/LTL/Stubborn/VisibleTransitionVisitor.h LTL/Algorithm/StubbornTarjanModelChecker.cpp ../include/LTL/Algorithm/StubbornTarjanModelChecker.h ../include/LTL/LTLMain.h ../src/LTL/LTLMain.cpp ../include/LTL/Structures/BitProductStateSet.h ../include/LTL/Structures/PairProductStateSet.h LTL/Stubborn/AutomatonStubbornSet.cpp ../include/LTL/Stubborn/AutomatonStubbornSet.h ../include/LTL/Structures/GuardInfo.h ../include/PetriEngine/Stubborn/NegatedStubbornSet.h ../include/LTL/Stubborn/ReducingSuccessorGenerator.h)
add_dependencies(verifypn-${ARCH_TYPE} glpk-ext spot-ext)

target_link_libraries(verifypn-${ARCH_TYPE} PRIVATE CTL PetriEngine PetriParse glpk LTL spot)

# add_executable(parse-benchmark-asctl ParseOnly.cpp)

 #target_link_libraries(parse-benchmark-asctl PetriEngine PetriParse)

if (VERIFYPN_MC_Simplification)
    target_link_libraries(verifypn-${ARCH_TYPE} PUBLIC pthread)
endif(VERIFYPN_MC_Simplification)
