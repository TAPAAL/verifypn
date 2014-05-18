#!/bin/bash

# This is the initialization script for the participation of TAPAAL
# untimed engine verifypn in the Petri net competition 2014.

# BK_EXAMINATION: it is a string that identifies your "examination"

export PATH="$PATH:/home/mcc/BenchKit/bin/"
VERIFYPN=$HOME/BenchKit/bin/verifypn
#VERIFYPN=/Users/srba/dev/sumoXMLparsing/verifypn-osx64
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
        for (( QUERY=1; QUERY<=$NUMBER; QUERY++ ))
	do
		echo
		echo "verifypn" $1 "-x" $QUERY "model.pnml" $2
		if [ $TIMEOUT = 0 ]; then
			$VERIFYPN $1 "-x" $QUERY "model.pnml" $2
		else
			timeout $TIMEOUT $VERIFYPN $1 "-x" $QUERY "model.pnml" $2
			RETVAL=$?
			if [ $RETVAL = 124 ] || [ $RETVAL =  125 ] || [ $RETVAL =  126 ] || [ $RETVAL =  127 ] || [ $RETVAL =  137 ] ; then
				echo -ne "CANNOT_COMPUTE\n"
			fi
		fi
	done
} 


case "$BK_EXAMINATION" in

	StateSpace)
		echo		
		echo "*****************************************"
		echo "*  TAPAAL performing StateSpace search  *"
		echo "*****************************************"
		$VERIFYPN -n -d -e model.pnml 
		;;

	ReachabilityComputeBounds)	
		echo		
		echo "***********************************************"
		echo "*  TAPAAL verifying ReachabilityComputeBounds *"
		echo "***********************************************"
		verify "-n -r 1" "ReachabilityComputeBounds.xml"
		;;

	ReachabilityDeadlock)
		echo		
		echo "**********************************************"
		echo "*  TAPAAL checking for ReachabilityDeadlock  *"
		echo "**********************************************"
		TIMEOUT=0
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
