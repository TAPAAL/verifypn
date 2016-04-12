#!/bin/bash

# This is the initialization script for the participation of TAPAAL
# untimed engine verifypn in the Petri net competition 2016.
# It uses a single core, processes first using heuristic search for
# a short time on all queries, then tries BFS and finally uses DFS
# on the not yet solved queries until we run out of time

# BK_EXAMINATION: it is a string that identifies your "examination"

export PATH="$PATH:/home/mcc/BenchKit/bin/"
#VERIFYPN=$HOME/BenchKit/bin/verifypn
#VERIFYPN=/Users/srba/dev/MCC-16/engines/verifypnCTL/verifypn-osx64
VERIFYPN=/Users/srba/dev/verifypnCTL/verifypn-osx64

#timeout for heuristic search
TIMEOUT1=10
#timeout for BFS search
TIMEOUT2=10
#timeout for DFS search
TIMEOUT3=10

STRATEGY1=""
STRATEGY2="-s BFS"
STRATEGY3="-s DFS"

#Allowed memory in kB
MEM="14500000"
ulimit -v $MEM

#STATISTICS="/usr/bin/time -f \"###%e,%M###\""
STATISTICS="/usr/local/bin/gtime -f \"###%e,%M###\""

if [ ! -f iscolored ]; then
    	echo "File 'iscolored' not found!"
else
    if [ "TRUE" = $(cat "iscolored") ]; then
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
        RQ=""
        for (( QUERY=1; QUERY<=$NUMBER; QUERY++ ))
	do
		echo
		echo "verifypn" $1 "-x" $QUERY $STRATEGY1 "model.pnml" $2
		gtimeout $TIMEOUT1 $STATISTICS $VERIFYPN "-x" $QUERY $1 $STRATEGY1 model.pnml $2
		RETVAL=$?
		if [ $RETVAL = 124 ] || [ $RETVAL =  125 ] || [ $RETVAL =  126 ] || \
                   [ $RETVAL =  127 ] || [ $RETVAL =  137 ] ; then
			RQ="$RQ $QUERY"
		fi
	done
        RQR=""
	for QUERY in $RQ ; do
  		echo
                echo "verifypn" $1 "-x" $QUERY $STRATEGY2 "model.pnml" $2
                gtimeout $TIMEOUT2 $STATISTICS $VERIFYPN -s BFS "-x" $QUERY $1 $STRATEGY2 model.pnml $2
                RETVAL=$?
                if [ $RETVAL = 124 ] || [ $RETVAL =  125 ] || [ $RETVAL =  126 ] || \
                   [ $RETVAL =  127 ] || [ $RETVAL =  137 ] ; then
                        RQR="$RQR $QUERY"
                fi	 
        done
	for QUERY in $RQR ; do
  		echo
                echo "verifypn" $1 "-x" $QUERY $STRATEGY3 "model.pnml" $2
                gtimeout $TIMEOUT3 $STATISTICS $VERIFYPN -s DFS "-x" $QUERY $STRATEGY3 $1 model.pnml $2
                #RETVAL=$?
                #if [ $RETVAL = 124 ] || [ $RETVAL =  125 ] || [ $RETVAL =  126 ] || \
                #   [ $RETVAL =  127 ] || [ $RETVAL =  137 ] ; then
		# echo -ne "CANNOT_COMPUTE\n"
                #fi	 
        done
} 

case "$BK_EXAMINATION" in

	StateSpace)
		echo		
		echo "*************************************************"
		echo "*  TAPAAL CLASSIC performing StateSpace search  *"
		echo "*************************************************"
		$VERIFYPN -n -d -e model.pnml 
		;;

	UpperBounds)	
		echo		
		echo "*****************************************"
		echo "*  TAPAAL CLASSIC verifying UpperBounds *"
		echo "*****************************************"
                STRATEGY1="-s DFS"
                STRATEGY2="-s BFS"
                STRATEGY3=""
		TIMEOUT1=3600
		TIMEOUT2=3600
		TIMEOUT3=3600
		verify "-n -r 1" "UpperBounds.xml"
		;;

	ReachabilityDeadlock)
		echo		
		echo "******************************************************"
		echo "*  TAPAAL CLASSIC checking for ReachabilityDeadlock  *"
		echo "******************************************************"
		TIMEOUT1=10
		TIMEOUT2=10
		TIMEOUT3=10
		verify "-n -r 1" "ReachabilityDeadlock.xml"
		;;

	ReachabilityCardinality)
		echo		
		echo "******************************************************"
		echo "*  TAPAAL CLASSIC verifying ReachabilityCardinality  *"
		echo "******************************************************"
		verify "-n -r 1" "ReachabilityCardinality.xml"
		;;

	ReachabilityFireability)
		echo		
		echo "******************************************************"
		echo "*  TAPAAL CLASSIC verifying ReachabilityFireability  *"
		echo "******************************************************"
		verify "-n -r 1" "ReachabilityFireability.xml"
		;;

	CTLCardinality)
		echo		
		echo "*********************************************"
		echo "*  TAPAAL CLASSIC verifying CTLCardinality  *"
		echo "*********************************************"
                STRATEGY1="-s DFS"
                STRATEGY2="-s DFS"
                STRATEGY3="-s DFS"
		TIMEOUT1=1
		TIMEOUT2=1
		TIMEOUT3=5
		verify "-ctl czero -n" "CTLCardinality.xml"
		;;

	CTLFireability)
		echo		
		echo "*********************************************"
		echo "*  TAPAAL CLASSIC verifying CTLFireability  *"
		echo "*********************************************"
                STRATEGY1="-s DFS"
                STRATEGY2="-s DFS"
                STRATEGY3="-s DFS"
		TIMEOUT1=1
		TIMEOUT2=1
		TIMEOUT3=5
		verify "-ctl czero -n" "CTLFireability.xml"
		;;

	*)
		echo "DO_NOT_COMPETE"	
		exit 0
		;;
esac
