#!/bin/bash

trap "echo 'Terminating all processes using: kill -15 -1'; kill -15 -1; sleep 1; echo 'Terminating all processes using: kill -9 -1'; kill -9 -1; echo 'Done with terminating all processes.'; exit" 1 2 3 15 


# This is the initialization script for the participation of TAPAAL
# untimed engine verifypn in the Petri net competition 2017.
# BK_EXAMINATION: it is a string that identifies your "examination"
MODEL_PATH=.

# Path to script directory
PREFIX=/home/mcc/BenchKit

# Verifypn
VERIFYPN=/home/mcc/BenchKit/bin/verifypn-linux64

# Tools
PAR_CMD=$PREFIX/bin/parallel/parallel
TIMEOUT_CMD=timeout
TIME_CMD="/usr/bin/time -v"

START_SCRIPT=$PREFIX/start.sh

#Allowed memory in kB
MEM="14500000"
ulimit -v $MEM
QREDMEM=$(echo "$MEM/2" | bc)

# Verification options
OPTIONS=""

# Stategies
STRATEGY_SEQ="-s DFS"

# Timeouts (in seconds)
#TIMEOUT_TOTAL=3600 # competition 1 hour 
TIMEOUT_TOTAL=$BK_TIME_CONFINEMENT 
TIMEOUT_SEQ_MIN=480 # competition 480 seconds 
TIMEOUT_PAR=60 # competition 1 min

echo "Total timeout: " $TIMEOUT_TOTAL

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


function verifyparallel {
    # Keep track of time passed (in seconds)
    SECONDS=0
    local NUMBER=`cat $MODEL_PATH/$CATEGORY | grep "<property>" | wc -l`
    QUERIES=( $(seq 1 $NUMBER) )

    # Step 1: Parallel
    echo "---------------------------------------------------"
    echo "            Step 1: Parallel processing            "
    echo "---------------------------------------------------"
    echo "Doing parallel verification of individual queries ($NUMBER in total)"
    echo "Each query is verified by ${#STRATEGIES_PAR[@]} parallel strategies for $TIMEOUT_PAR seconds"

    for Q in ${QUERIES[@]}; do
        echo "------------------- QUERY ${Q} ----------------------"
        # Execute verifypn on all parallel strategies
        # All processes are killed if one process provides an answer 
        c="$VERIFYPN $OPTIONS {1} $MODEL_PATH/model.pnml $MODEL_PATH/$CATEGORY -x $Q"
        step1="$($PAR_CMD --line-buffer --halt now,success=1 --timeout $TIMEOUT_PAR --xapply\
            $START_SCRIPT {2} $c\
            ::: "${STRATEGIES_PAR[@]}" :::+ "${PRE[@]}" 2>&1)"

        if [[ $? == 0 ]]; then
            unset QUERIES[$Q-1]
            echo "Solution found"
        else
            echo "No solution found"
        fi
        FORMULA_RESULT="$(echo "$step1"|grep -m 1 FORMULA)"
        
        if [[ -n "$FORMULA_RESULT" ]]; then 
            echo "$step1" | sed "/$FORMULA_RESULT/d"
        else 
            echo "$step1"
        fi
        echo 
        echo "$FORMULA_RESULT"
    done

    # Exit if all queries are answered
    if [[ ${#QUERIES[@]} == 0 ]]; then echo "All queries are solved" ; exit; fi
    

    # Step 2: Sequential
    echo "---------------------------------------------------"
    echo "           Step 2: Sequential processing           "
    echo "---------------------------------------------------"
    echo "Remaining queries are verified sequentially."
    echo "Each query is verified for a dynamic timeout (at least $TIMEOUT_SEQ_MIN seconds)"
    
    # Count the number of remaining queries to try solving sequentially
    REMAINING_SEQ=${#QUERIES[@]}

    for Q in ${QUERIES[@]}; do
        echo "------------------- QUERY ${Q} ----------------------"
        # Calculate remaining time
        REMAINING_TIME=$(echo "$TIMEOUT_TOTAL - $SECONDS" | bc)
        TIMEOUT_SEQ=$(echo "$REMAINING_TIME / $REMAINING_SEQ" | bc)
        if [[ "$TIMEOUT_SEQ_MIN" -gt "$TIMEOUT_SEQ" ]]; then TIMEOUT_SEQ=$TIMEOUT_SEQ_MIN; fi
        if [[ "$TIMEOUT_SEQ" -gt "$REMAINING_TIME" ]]; then TIMEOUT_SEQ=$REMAINING_TIME; fi
        TIMEOUT_SEQ=$((TIMEOUT_SEQ+2))
 
        # Execute verifypn on sequential strategy
        echo "Running query $Q for $TIMEOUT_SEQ seconds. Remaining: $REMAINING_SEQ queries and $REMAINING_TIME seconds"
        $TIME_CMD $TIMEOUT_CMD $TIMEOUT_SEQ $VERIFYPN $OPTIONS $STRATEGY_SEQ $MODEL_PATH/model.pnml $MODEL_PATH/$CATEGORY -x $Q
        RETVAL=$?

        if [[ $RETVAL == 0 ]]; then
            unset QUERIES[$Q-1]
            echo "Solution found"
        elif [[ $RETVAL == 3 && "$STRATEGY_SEQ" != *"-q 0"* ]]; then
            echo "Verifypn has exit with error. Re-trying without query reduction"
            echo "Running query $Q for $TIMEOUT_SEQ seconds. Remaining: $REMAINING_SEQ queries and $REMAINING_TIME seconds"
            $TIME_CMD $TIMEOUT_CMD $TIMEOUT_SEQ $VERIFYPN $OPTIONS $STRATEGY_SEQ -q 0 $MODEL_PATH/model.pnml $MODEL_PATH/$CATEGORY -x $Q
            if [[ $? == 0 ]]; then
                unset QUERIES[$Q-1]
                echo "Solution found (without query reduction)"
            else
                echo "No solution found (without query reduction)"
            fi
        else
            echo "No solution found"
        fi

        REMAINING_SEQ=$((REMAINING_SEQ - 1))
    done
    
    # Exit if all queries are answered
    if [[ ${#QUERIES[@]} == 0 ]]; then echo "All queries are solved" ; exit; fi


    # Step 3: Multiquery
    
    echo "---------------------------------------------------"
    echo "           Step 3: Multiquery processing           "
    echo "---------------------------------------------------"
    REMAINING_TIME=$(echo "$TIMEOUT_TOTAL - $SECONDS"|bc)
    REMAINING_TIME=$((REMAINING_TIME+2))

    echo "Remaining ${#QUERIES[@]} queries are solved using multiquery"
    echo "Time remaining: $REMAINING_TIME seconds of the initial $TIMEOUT_TOTAL seconds" 
    
    # Join remaining query indexes in comma separated string
    MULTIQUERY_INPUT=$(sed -e "s/ /,/g" <<< ${QUERIES[@]})
    
    echo "Running multiquery on -x $MULTIQUERY_INPUT for $REMAINING_TIME seconds" 
    $TIME_CMD $TIMEOUT_CMD $REMAINING_TIME $VERIFYPN $STRATEGY_SEQ $OPTIONS -p $MODEL_PATH/model.pnml $MODEL_PATH/$CATEGORY -x $MULTIQUERY_INPUT 
    
    echo "End of script."
}

case "$BK_EXAMINATION" in

    StateSpace)
        echo        
        echo "*****************************************"
        echo "*  TAPAAL performing StateSpace search  *"
        echo "*****************************************"
        $TIME_CMD $TIMEOUT_CMD $TIMEOUT_TOTAL $VERIFYPN -p -q 0 -e -s BFS $MODEL_PATH/model.pnml 
        ;;

    UpperBounds)    
        echo        
        echo "**********************************"
        echo "*  TAPAAL verifying UpperBounds  *"
        echo "**********************************" 
        CATEGORY="UpperBounds.xml"
        STRATEGY_SEQ="-s BFS -r 1 -d 300 -p -q 0"
        NUMBER=`cat $MODEL_PATH/$CATEGORY | grep "<property>" | wc -l`
        $TIME_CMD $TIMEOUT_CMD $TIMEOUT_TOTAL $VERIFYPN $STRATEGY_SEQ $OPTIONS $MODEL_PATH/model.pnml $MODEL_PATH/$CATEGORY -x "$(seq -s , 1 $NUMBER)" 
        ;;

    ReachabilityDeadlock)
        echo        
        echo "**********************************************"
        echo "*  TAPAAL checking for ReachabilityDeadlock  *"
        echo "**********************************************"
        PRE[0]="$MEM"
        PRE[1]="$MEM"
        PRE[2]="$MEM"
        PRE[3]="$MEM"
        STRATEGIES_PAR[0]="-s DFS -d 10 --siphon-trap 30 -q 0"
        STRATEGIES_PAR[1]="-s BFS -d 10 -q 0"
        STRATEGIES_PAR[2]="-s DFS -d 20 -q 0"
        STRATEGIES_PAR[3]="-s RDFS -d 5 -q 0"
        STRATEGY_SEQ="-s DFS -d 200 -q 0"
        CATEGORY="ReachabilityDeadlock.xml"
        verifyparallel 
        ;;

    ReachabilityCardinality)
        echo        
        echo "**********************************************"
        echo "*  TAPAAL verifying ReachabilityCardinality  *"
        echo "**********************************************"
        PRE[0]="$QREDMEM"
        PRE[1]="$MEM"
        PRE[2]="$MEM"
        PRE[3]="$MEM"
        STRATEGIES_PAR[0]="-s BestFS -d 10"
        STRATEGIES_PAR[1]="-s BestFS -q 0 -d 10"
        STRATEGIES_PAR[2]="-s BFS -q 0 -d 10"
        STRATEGIES_PAR[3]="-s DFS -q 0 -d 10"
        STRATEGY_SEQ="-s DFS -d 100 -q 100"
        CATEGORY="ReachabilityCardinality.xml"
        verifyparallel 
        ;;

    ReachabilityFireability)
        echo        
        echo "**********************************************"
        echo "*  TAPAAL verifying ReachabilityFireability  *"
        echo "**********************************************"
        PRE[0]="$QREDMEM"
        PRE[1]="$MEM"
        PRE[2]="$MEM"
        PRE[3]="$MEM"
        STRATEGIES_PAR[0]="-s BestFS -d 10"
        STRATEGIES_PAR[1]="-s BestFS -q 0 -d 10"
        STRATEGIES_PAR[2]="-s BFS -q 0 -d 10"
        STRATEGIES_PAR[3]="-s DFS -q 0 -d 10"
        STRATEGY_SEQ="-s DFS -d 100 -q 100"
        CATEGORY="ReachabilityFireability.xml"
        verifyparallel 
        ;;

    CTLCardinality)
        echo        
        echo "*************************************"
        echo "*  TAPAAL verifying CTLCardinality  *"
        echo "*************************************"
        PRE[0]="$QREDMEM"
        PRE[1]="$MEM"
        STRATEGIES_PAR[0]="-s DFS -d 10"
        STRATEGIES_PAR[1]="-s DFS -q 0 -d 10"
        STRATEGY_SEQ="-s DFS -d 100 -q 100"
        CATEGORY="CTLCardinality.xml"
        verifyparallel
        ;;

    CTLFireability)
        echo        
        echo "*************************************"
        echo "*  TAPAAL verifying CTLFireability  *"
        echo "*************************************"
        PRE[0]="$QREDMEM"
        PRE[1]="$MEM"
        STRATEGIES_PAR[0]="-s DFS -d 10"
        STRATEGIES_PAR[1]="-s DFS -q 0 -d 10"
        STRATEGY_SEQ="-s DFS -d 100 -q 100"
        CATEGORY="CTLFireability.xml"
        verifyparallel
        ;;

    *)
        echo "DO_NOT_COMPETE"  
        exit 0
        ;;
esac



