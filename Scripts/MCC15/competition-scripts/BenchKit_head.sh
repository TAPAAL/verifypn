#!/bin/bash

# This is the initialization script for the participation of TAPAAL
# untimed engine verifypn in the Petri net competition 2015.

# BK_EXAMINATION: it is a string that identifies your "examination"
# BK_TOOL: it is the name of the TAPAAL tool variant to be invoked
# It should be: classicSEQ, classicMC, ontheflySEQ, ontheflyMC or ontheflyPAR

PREFIX=$HOME/BenchKit

case "$BK_TOOL" in

	classicSEQ)
		echo "---> " $BK_TOOL " --- TAPAAL Classic Sequential"
		$PREFIX/classicSEQ.sh
		;;
	classicMC)
		echo "---> " $BK_TOOL " --- TAPAAL Classic Multicore"
		$PREFIX/classicMC.sh
		;;
	ontheflySEQ)
		echo "---> " $BK_TOOL " --- TAPAAL On-the-Fly Sequential"
		$PREFIX/ontheflySEQ.sh
		;;
	ontheflyMC)
		echo "---> " $BK_TOOL " --- TAPAAL On-the-Fly Multicore"
		$PREFIX/ontheflyMC.sh
		;;
	ontheflyPAR)
		echo "---> " $BK_TOOL " --- TAPAAL On-the-Fly Parallel"
		$PREFIX/ontheflyPAR.sh
		;;
	*)
		echo "---> Error: Unrecognized BK_TOOL name !!!"	
		exit 1
		;;
esac
