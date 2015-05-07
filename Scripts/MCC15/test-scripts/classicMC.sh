#!/bin/bash

# This is the initialization script for the participation of TAPAAL
# untimed engine verifypn in the Petri net competition 2015.
# It uses a number of available cores.

# BK_EXAMINATION: it is a string that identifies your "examination"

export PATH="$PATH:/home/mcc/BenchKit/bin/"
#VERIFYPN=$HOME/BenchKit/bin/verifypn
VERIFYPN=/Users/srba/dev/MCC-15/Testing/verifypn
VERIFYPNBOUNDS=/Users/srba/dev/MCC-15/Testing/verifypn-bounds
TIMEOUT=10

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
	parallel --will-cite -j2 -- "gtimeout $TIMEOUT $VERIFYPN $1 "-x" {} "model.pnml" $2 ; RETVAL=\$? ;\
		if [ \$RETVAL = 124 ] || [ \$RETVAL =  125 ] || [ \$RETVAL =  126 ] || [ \$RETVAL =  127 ] || [ \$RETVAL =  137 ] ; then echo -ne \"CANNOT_COMPUTE\n\"; fi"
} 

function verify-bounds {
	if [ ! -f $2 ]; then
    		echo "File '$2' not found!" 
		exit 1 
	fi
	local NUMBER=`cat $2 | grep "<property>" | wc -l`

        seq 1 $NUMBER | 
	parallel --will-cite -j2 -- "gtimeout $TIMEOUT $VERIFYPNBOUNDS $1 "-x" {} "model.pnml" $2 ; RETVAL=\$? ;\
		if [ \$RETVAL = 124 ] || [ \$RETVAL =  125 ] || [ \$RETVAL =  126 ] || [ \$RETVAL =  127 ] || [ \$RETVAL =  137 ] ; then echo -ne \"CANNOT_COMPUTE\n\"; fi"
} 



case "$BK_EXAMINATION" in

	StateSpace)
		echo		
		echo "*****************************************"
		echo "*  TAPAAL performing StateSpace search  *"
		echo "*****************************************"
		gtimeout $TIMEOUT $VERIFYPN -n -d -e model.pnml 
		;;

	ReachabilityComputeBounds)	
		echo		
		echo "***********************************************"
		echo "*  TAPAAL verifying ReachabilityComputeBounds *"
		echo "***********************************************"
		verify "-n -r 1" "ReachabilityComputeBounds.xml"
		;;

	ReachabilityBounds)	
		echo		
		echo "***********************************************"
		echo "*  TAPAAL verifying ReachabilityBounds *"
		echo "***********************************************"
		verify-bounds "-n -r 1" "ReachabilityBounds.xml"
		;;

	ReachabilityDeadlock)
		echo		
		echo "**********************************************"
		echo "*  TAPAAL checking for ReachabilityDeadlock  *"
		echo "**********************************************"
		TIMEOUT=10
		verify "-n -r 1" "ReachabilityDeadlock.xml"
		;;

	ReachabilityCardinality)
		echo		
		echo "**********************************************"
		echo "*  TAPAAL verifying ReachabilityCardinality  *"
		echo "**********************************************"
		verify "-n -r 1" "ReachabilityCardinality.xml"
		;;

	ReachabilityFireability)
		echo		
		echo "**********************************************"
		echo "*  TAPAAL verifying ReachabilityFireability  *"
		echo "**********************************************"
		verify "-n -r 1" "ReachabilityFireability.xml"
		;;

	ReachabilityFireabilitySimple)
                echo
                echo "****************************************************"
                echo "*  TAPAAL verifying ReachabilityFireabilitySimple  *"
                echo "****************************************************"
                verify "-n -r 1" "ReachabilityFireabilitySimple.xml"
                ;;

	*)
		echo "DO_NOT_COMPETE"	
		exit 0
		;;
esac
