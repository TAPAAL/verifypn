#!/bin/bash

# This is the initialization script for the participation of TAPAAL
# classical untimed engine verifypn in the Petri net competition 2015.
# It uses four available cores.

# BK_EXAMINATION: it is a string that identifies your "examination"

export PATH="$PATH:/home/mcc/BenchKit/bin/classic"
VERIFYPN=$HOME/BenchKit/bin/classic/verifypn
VERIFYPNBOUNDS=$HOME/BenchKit/bin/classic/verifypn-bounds
#VERIFYPN=/home/mcc/BenchKit/bin/verifypn
TIMEOUT=1000
TOOLNAME=classicMC

if [ ! -f iscolored ]; then
    	echo "File 'iscolored' not found!"
else
	if [ "TRUE" = `cat iscolored` ]; then
		echo "TAPAAL does not support colored nets."
		echo "DO_NOT_COMPETE" 
		exit 0
	fi
fi

if [ ! -f model.pnml ]; then
    	echo "File 'model.pnml' not found!"
	exit 1
fi

function verify {
	if [ ! -f $2 ]; then
    		echo "File '$2' not found!" 
		exit 1 
	fi
	local NUMBER=`cat $2 | grep "<property>" | wc -l`

        seq 1 $NUMBER | 
	parallel --gnu -j4 -- "timeout $TIMEOUT $VERIFYPN $1 "-x" {} "model.pnml" $2 ; RETVAL=\$? ;\
		if [ \$RETVAL = 124 ] || [ \$RETVAL =  125 ] || [ \$RETVAL =  126 ] || [ \$RETVAL =  127 ] || [ \$RETVAL =  137 ] ; then echo -ne \"CANNOT_COMPUTE\n\"; fi"
} 

function verify-bounds {
	if [ ! -f $2 ]; then
    		echo "File '$2' not found!" 
		exit 1 
	fi
	local NUMBER=`cat $2 | grep "<property>" | wc -l`

        seq 1 $NUMBER | 
	parallel --gnu -j4 -- "timeout $TIMEOUT $VERIFYPNBOUNDS $1 "-x" {} "model.pnml" $2 ; RETVAL=\$? ;\
		if [ \$RETVAL = 124 ] || [ \$RETVAL =  125 ] || [ \$RETVAL =  126 ] || [ \$RETVAL =  127 ] || [ \$RETVAL =  137 ] ; then echo -ne \"CANNOT_COMPUTE\n\"; fi"
} 

case "$BK_EXAMINATION" in

	StateSpace)
		echo		
		echo "*****************************************"
		echo "  TAPAAL " $TOOLNAME " performing StateSpace search"
		echo "*****************************************"
		$VERIFYPN -n -d -e model.pnml 
		;;

	ReachabilityComputeBounds)	
		echo		
		echo "***********************************************"
		echo "  TAPAAL " $TOOLNAME " verifying ReachabilityComputeBounds"
		echo "***********************************************"
		verify "-n -r 1" "ReachabilityComputeBounds.xml"
		;;

	ReachabilityBounds)	
		echo		
		echo "***********************************************"
		echo "  TAPAAL " $TOOLNAME " verifying ReachabilityBounds"
		echo "***********************************************"
		verify-bounds "-n -r 1" "ReachabilityBounds.xml"
		;;

	ReachabilityDeadlock)
		echo		
		echo "**********************************************"
		echo "  TAPAAL " $TOOLNAME " checking for ReachabilityDeadlock"
		echo "**********************************************"
		TIMEOUT=0
		verify "-n -r 1" "ReachabilityDeadlock.xml"
		;;

	ReachabilityCardinality)
		echo		
		echo "**********************************************"
		echo "  TAPAAL " $TOOLNAME " verifying ReachabilityCardinality"
		echo "**********************************************"
		verify "-n -r 1" "ReachabilityCardinality.xml"
		;;

	ReachabilityFireability)
		echo		
		echo "**********************************************"
		echo "  TAPAAL " $TOOLNAME " verifying ReachabilityFireability"
		echo "**********************************************"
		verify "-n -r 1" "ReachabilityFireability.xml"
		;;

	ReachabilityFireabilitySimple)
                echo
                echo "****************************************************"
                echo "  TAPAAL " $TOOLNAME " verifying ReachabilityFireabilitySimple"
                echo "****************************************************"
                verify "-n -r 1" "ReachabilityFireabilitySimple.xml"
                ;;

	*)
		echo "DO_NOT_COMPETE"	
		exit 0
		;;
esac
