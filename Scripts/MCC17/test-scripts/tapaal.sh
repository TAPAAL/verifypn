#!/bin/bash

# This is the initialization script for the participation of TAPAAL
# untimed engine verifypn in the Petri net competition 2017.
# BK_EXAMINATION: it is a string that identifies your "examination"
MODEL_PATH=./

# Path to script directory
PREFIX=$HOME/Datalogi/speciale/tools/flagship/Scripts/MCC17/test-scripts

# Verifypn executable
VERIFYPN=$HOME/Datalogi/speciale/tools/flagship/verifypn-osx64

# Tools
PAR=$PREFIX/../bin/parallel
TIMEOUT=gtimeout

#Allowed memory in kB
MEM="14500000"
ulimit -v $MEM

# Default verification options
OPTIONS="-n -o"

# Default stategies
STRATEGY_SEQ="-s DFS"
STRATEGIES_PAR=()

# Default timeouts
TIMEOUT_TOTAL=60
TIMEOUT_SEQ=30
TIMEOUTS_PAR=()
DEFAULT_TIMEOUT_PAR=1

function verifyparallel {
    # Keep track of time passed (in seconds)
    SECONDS=0
    local NUMBER=`cat $MODEL_PATH/$CATEGORY | grep "<property>" | wc -l`
    QUERIES=( $(seq 1 $NUMBER) )

    # Step 1: Parallel
    echo
    echo "--- Step 1: Parallel processing"  
    for Q in ${QUERIES[@]}; do
        # Calculate remaining time
        REMAINING_TIME=$(echo "$TIMEOUT_TOTAL - $SECONDS"|bc)
        # Execute verifypn on all parallel strategies
        # All processes are killed if one process provides an answer 
        step1="$($PAR --halt now,success=1 --xapply\
            eval $TIMEOUT {1} $VERIFYPN $OPTIONS {2} $MODEL_PATH/model.pnml $MODEL_PATH/$CATEGORY -x $Q ::: \
            "${TIMEOUTS_PAR[@]}" ::: "${STRATEGIES_PAR[@]}" \
            2>/dev/null)"

        # Output the answer, if any
        ANS=$(echo "$step1" | grep -m 1 FORMULA)
        if [[ -n "$ANS" ]]; then
            echo "$ANS"
            unset QUERIES[$Q-1]
        fi
    done

    echo
    echo "--- Step 2: Sequential processing" 
    
    # Exit if all queries are answered
    if [[ ${#QUERIES[@]} == 0 ]]; then echo "All queries solved in Step 1"; exit; fi

    # Step 2: Sequential    
    for Q in ${QUERIES[@]}; do
        # Calculate remaining time
        REMAINING_TIME=$(echo "$TIMEOUT_TOTAL - $SECONDS" | bc)
        TIMEOUT_SEQ=$(echo "$REMAINING_TIME / ${#QUERIES[@]}" | bc)
        if [[ $TIMEOUT_SEQ == 0 ]]; then exit; fi
        
        # Execute verifypn on sequential strategy
        echo "Running query $Q for $TIMEOUT_SEQ seconds. Remaining time/queries: $REMAINING_TIME/${#QUERIES[@]}" 
        step2=$($TIMEOUT $TIMEOUT_SEQ $VERIFYPN $OPTIONS $STRATEGY_SEQ $MODEL_PATH/model.pnml $MODEL_PATH/$CATEGORY -x $Q)

        # Output the answer, if any
        ANS=$(echo "$step2" | grep -m 1 FORMULA)
        if [[ -n "$ANS" ]]; then
            echo "$ANS"
        fi
        
        # Remove query regardless of it being solved or not
        unset QUERIES[$Q-1]
    done

    REMAINING_TIME=$(echo "$TIMEOUT_TOTAL - $SECONDS"|bc)
    echo "End of script. Remaining time: $REMAINING_TIME seconds"
}


case "$BK_EXAMINATION" in

    StateSpace)
       echo        
       echo "*****************************************"
       echo "*  TAPAAL performing StateSpace search  *"
       echo "*****************************************"
        $TIMEOUT $TIMEOUT_TOTAL $VERIFYPN -o -n -p -q 0 -e $MODEL_PATH/model.pnml 
        ;;

        UpperBounds)    
       echo     
       echo "*****************************************"
       echo "*  TAPAAL CLASSIC verifying UpperBounds *"
       echo "*****************************************" 
        CATEGORY="UpperBounds.xml"
        STRATEGY_SEQ="-s BFS"
        OPTIONS="-n -o -r 2 -p -q 0"
        NUMBER=`cat $MODEL_PATH/$CATEGORY | grep "<property>" | wc -l`
        $TIMEOUT $TIMEOUT_TOTAL $VERIFYPN $STRATEGY_SEQ $OPTIONS $MODEL_PATH/model.pnml $MODEL_PATH/$CATEGORY -x "$(seq -s , 1 $NUMBER)" 
        ;;

    ReachabilityDeadlock)
       echo        
       echo "**********************************************"
       echo "*  TAPAAL checking for ReachabilityDeadlock  *"
       echo "**********************************************"
        STRATEGIES_PAR[0]="-s DFS --siphon-trap $DEFAULT_TIMEOUT_PAR -q 0"
        STRATEGIES_PAR[1]="-s BFS -q 0"
        STRATEGIES_PAR[2]="-s DFS -q 0"
        STRATEGIES_PAR[3]="-s RDFS -q 0"

        TIMEOUTS_PAR[0]=$DEFAULT_TIMEOUT_PAR
        TIMEOUTS_PAR[1]=$DEFAULT_TIMEOUT_PAR
        TIMEOUTS_PAR[2]=$DEFAULT_TIMEOUT_PAR
        TIMEOUTS_PAR[3]=$DEFAULT_TIMEOUT_PAR

        STRATEGY_SEQ="-s DFS"
        
        CATEGORY="ReachabilityDeadlock.xml"
        verifyparallel 
        ;;

    ReachabilityCardinality)
       echo        
       echo "**********************************************"
       echo "*  TAPAAL verifying ReachabilityCardinality  *"
       echo "**********************************************"
        STRATEGIES_PAR[0]="-s BestFS"
        STRATEGIES_PAR[1]="-s BestFS -q 0"
        STRATEGIES_PAR[2]="-s BFS -q 0"
        STRATEGIES_PAR[3]="-s DFS -q 0"

        TIMEOUTS_PAR[0]=$DEFAULT_TIMEOUT_PAR
        TIMEOUTS_PAR[1]=$DEFAULT_TIMEOUT_PAR
        TIMEOUTS_PAR[2]=$DEFAULT_TIMEOUT_PAR
        TIMEOUTS_PAR[3]=$DEFAULT_TIMEOUT_PAR

        STRATEGY_SEQ="-s DFS"

        CATEGORY="ReachabilityCardinality.xml"
        verifyparallel 
        ;;

    ReachabilityFireability)
      echo        
       echo "**********************************************"
       echo "*  TAPAAL verifying ReachabilityFireability  *"
       echo "**********************************************"
        STRATEGIES_PAR[0]="-s BestFS"
        STRATEGIES_PAR[1]="-s BestFS -q 0"
        STRATEGIES_PAR[2]="-s BFS -q 0"
        STRATEGIES_PAR[3]="-s DFS -q 0"

        TIMEOUTS_PAR[0]=$DEFAULT_TIMEOUT_PAR
        TIMEOUTS_PAR[1]=$DEFAULT_TIMEOUT_PAR
        TIMEOUTS_PAR[2]=$DEFAULT_TIMEOUT_PAR
        TIMEOUTS_PAR[3]=$DEFAULT_TIMEOUT_PAR
      
        STRATEGY_SEQ="-s DFS"

        CATEGORY="ReachabilityFireability.xml"
        verifyparallel 
        ;;

    CTLCardinality)
       echo        
       echo "*************************************"
       echo "*  TAPAAL verifying CTLCardinality  *"
       echo "*************************************"

        STRATEGIES_PAR[0]="-s DFS"
        STRATEGIES_PAR[1]="-s DFS -q 0"
        STRATEGIES_PAR[2]="-s BFS -q 0"

        TIMEOUTS_PAR[0]=$DEFAULT_TIMEOUT_PAR
        TIMEOUTS_PAR[1]=$DEFAULT_TIMEOUT_PAR
        TIMEOUTS_PAR[2]=$DEFAULT_TIMEOUT_PAR

        STRATEGY_SEQ="-s DFS"
        CATEGORY="CTLCardinality.xml"
        verifyparallel
        ;;

    CTLFireability)
       echo        
       echo "*************************************"
       echo "*  TAPAAL verifying CTLFireability  *"
       echo "*************************************"
        
        STRATEGIES_PAR[0]="-s DFS"
        STRATEGIES_PAR[1]="-s DFS -q 0"
        STRATEGIES_PAR[2]="-s BFS -q 0"

        TIMEOUTS_PAR[0]=$DEFAULT_TIMEOUT_PAR
        TIMEOUTS_PAR[1]=$DEFAULT_TIMEOUT_PAR
        TIMEOUTS_PAR[2]=$DEFAULT_TIMEOUT_PAR

        STRATEGY_SEQ="-s DFS"
        CATEGORY="CTLFireability.xml"
        verifyparallel
        ;;

    *)
        echo "DO_NOT_COMPETE"   
        exit 0
        ;;
esac