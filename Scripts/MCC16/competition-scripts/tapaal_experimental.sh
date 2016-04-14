#!/bin/bash

# This is the initialization script for the participation of TAPAAL
# untimed engine verifypn in the Petri net competition 2016.
# It uses a single core, processes first using heuristic search for
# a short time on all queries, then tries BFS and finally uses DFS
# on the not yet solved queries until we run out of time

# BK_EXAMINATION: it is a string that identifies your "examination"

export PATH="$PATH:/home/mcc/BenchKit/bin/"
VERIFYPN=$HOME/BenchKit/bin/verifypn-experimental-linux64
#VERIFYPN=/Users/srba/dev/MCC-16/engines/verifypnCTL/verifypn-osx64
#VERIFYPN=/Users/srba/dev/verifypnCTL/verifypn-osx64

#Allowed memory in kB
MEM="14500000"
ulimit -v $MEM

#STATISTICS="/usr/bin/time -f \"###%e,%M###\""
#STATISTICS="/usr/local/bin/gtime -f \"###%e,%M###\""

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


# timout, queries, options, qfile
function verifyall {
	if [[ $2 == "" ]] ; then
    		echo "No more queries" 
		exit 1 
	fi
        
        QS=$(echo $2 | sed 's/ /,/g' ) 

        echo "verifypnall" $3 "-x" "$QS" "model.pnml" $4
        timeout $1 $VERIFYPN $3 "-x" "$QS" model.pnml $4	 
}

# timout, queries, options, qfile
function verify {
        if [[ $2 == "" ]] ; then
            echo "No more queries"
            exit 1
        fi
        RQ=""
        QS=$(echo $2)
	for QUERY in $QS ; do
  		echo
                echo "verifypn" $3 "-x" "$QUERY" "model.pnml" $4
                timeout $1 $VERIFYPN $3 "-x" $QUERY model.pnml $4
                RETVAL=$?
                if [ $RETVAL -ge 4 ] || [ $RETVAL -lt 0 ] ; then
                        RQ="$RQ $QUERY"
                fi	 
        done
        LIST="$RQ"
} 

function getlist
{
    NUMBER=`cat $1 | grep "<property>" | wc -l`

    QS=""
    for (( QUERY=1; QUERY<=$NUMBER; QUERY++ )) ;
    do
        QS="$QS $QUERY"
    done
    echo $QS
}

case "$BK_EXAMINATION" in

	StateSpace)
		echo		
		echo "****************************************************"
		echo "* TAPAAL Experimental performing StateSpace search *"
		echo "****************************************************"
		$VERIFYPN -n -d -e model.pnml 
                exit 0 
		;;

	UpperBounds)	
		echo		
		echo "********************************************"
		echo "* TAPAAL Experimental verifying UpperBounds*"
		echo "********************************************"
                LIST=$(getlist "UpperBounds.xml")
                verify 60 "$LIST" "-d -n -r 1 -s BFS" "UpperBounds.xml"
                verifyall 7200 "$LIST" "-d -n -r 1 -s BFS" "UpperBounds.xml"
                exit 0 
		;;

	ReachabilityDeadlock)
		echo		
		echo "*********************************************************"
		echo "* TAPAAL Experimental checking for ReachabilityDeadlock *"
		echo "*********************************************************"
                LIST=$(getlist "ReachabilityDeadlock.xml")
                verify 60 "$LIST" "-d -n -r 1 -s DFS" "ReachabilityDeadlock.xml"
                verify 60 "$LIST" "-d -n -r 1 -s BFS" "ReachabilityDeadlock.xml"
                verifyall 7200 "$LIST" "-d -n -r 1 -s DFS" "ReachabilityDeadlock.xml"
                exit 0 
		;;

	ReachabilityCardinality)
		echo		
		echo "*********************************************************"
		echo "* TAPAAL Experimental verifying ReachabilityCardinality *"
		echo "*********************************************************"
                LIST=$(getlist "ReachabilityCardinality.xml")
                verify 10 "$LIST" "-n -s OverApprox" "ReachabilityCardinality.xml"
                verify 60 "$LIST" "-d -n -r 1 -s BestFS" "ReachabilityCardinality.xml"
                verify 30 "$LIST" "-d -n -r 1 -s DFS" "ReachabilityCardinality.xml"
                verify 30 "$LIST" "-d -n -r 1 -s BFS" "ReachabilityCardinality.xml"
                verifyall 7200 "$LIST" "-d -n -r 1 -s BestFS" "ReachabilityCardinality.xml"
                exit 0 
		;;

	ReachabilityFireability)
		echo		
		echo "*********************************************************"
		echo "* TAPAAL Experimental verifying ReachabilityFireability *"
		echo "*********************************************************"
                LIST=$(getlist "ReachabilityFireability.xml")
                verify 10 "$LIST" "-n -s OverApprox" "ReachabilityFireability.xml"
                verify 60 "$LIST" "-d -n -r 1 -s BestFS" "ReachabilityFireability.xml"
                verify 30 "$LIST" "-d -n -r 1 -s DFS" "ReachabilityFireability.xml"
                verify 30 "$LIST" "-d -n -r 1 -s BFS" "ReachabilityFireability.xml"
                verifyall 7200 "$LIST" "-d -n -r 1 -s BestFS" "ReachabilityFireability.xml"
                exit 0    
		;;

	*)
		echo "DO_NOT_COMPETE"	
		exit 0
		;;
esac
