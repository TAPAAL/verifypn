#!/bin/bash

# This is the initialization script for the participation of TAPAAL
# untimed engine verifypn in the Petri net competition 2014.

# BK_EXAMINATION: it is a string that identifies your "examination"
# BK_TOOL: it is the name of the TAPAAL tool variant to be invoked

PREFIX=/Users/srba/dev/MCC-16/

case "$BK_TOOL" in
	tapaal)
		echo "---> " $BK_TOOL " --- TAPAAL Classic"
		$PREFIX/tapaal.sh
                ;;
	tapaalSEQ)
		echo "---> " $BK_TOOL " --- TAPAAL Classic Sequential"
		$PREFIX/tapaalSEQ.sh
		;;
	tapaalEXP)
		echo "---> " $BK_TOOL " --- TAPAAL Classic Experimential"
		$PREFIX/tapaalEXP.sh
		;;
	tapaalEXP2)
		echo "---> " $BK_TOOL " --- TAPAAL Classic Experimential2"
		$PREFIX/tapaalEXP2.sh
                ;;
	*)
		echo "---> Wrong TAPAAL tool variant name"	
		exit 0
		;;
esac
