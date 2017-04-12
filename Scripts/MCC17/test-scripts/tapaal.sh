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

# Verification options
OPTIONS="-n"

# Stategies
STRATEGY_SEQ="-s DFS"

# Timeouts (in seconds)
TIMEOUT_TOTAL=3600 # competition 1 hour
TIMEOUT_SEQ_MIN=480 # competition 8 min
DEFAULT_TIMEOUT_PAR=60 # competition 1 min

function verifyparallel {
    # Keep track of time passed (in seconds)
    SECONDS=0
    local NUMBER=`cat $MODEL_PATH/$CATEGORY | grep "<property>" | wc -l`
    QUERIES=( $(seq 1 $NUMBER) )

    #################################
    # Step 1: Parallel
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
        if [[ $? == 0 ]]; then
            echo "$step1" | grep -m 1 FORMULA 
            unset QUERIES[$Q-1]
        fi
    done

    # Exit if all queries are answered
    if [[ ${#QUERIES[@]} == 0 ]]; then echo "All queries are solved" ; exit; fi
    
    #################################
    # Step 2: Sequential
    echo "--- Step 2: Sequential processing" 

    # Count the number of remaining queries to try solving sequentially
    REMAINING_SEQ=${#QUERIES[@]}

    for Q in ${QUERIES[@]}; do
        # Calculate remaining time
        REMAINING_TIME=$(echo "$TIMEOUT_TOTAL - $SECONDS" | bc)
        TIMEOUT_SEQ=$(echo "$REMAINING_TIME / $REMAINING_SEQ" | bc)
        if [[ "$TIMEOUT_SEQ_MIN" -gt "$TIMEOUT_SEQ" ]]; then TIMEOUT_SEQ=$TIMEOUT_SEQ_MIN; fi
        
        # Execute verifypn on sequential strategy
        echo "Running query $Q for $TIMEOUT_SEQ seconds. Remaining: $REMAINING_SEQ queries and $REMAINING_TIME seconds"
        step2=$($TIMEOUT $TIMEOUT_SEQ $VERIFYPN $OPTIONS $STRATEGY_SEQ $MODEL_PATH/model.pnml $MODEL_PATH/$CATEGORY -x $Q)

        # Output the answer, if any
        if [[ $? == 0 ]]; then
            echo "$step2" | grep -m 1 FORMULA 
            unset QUERIES[$Q-1]
        fi
        REMAINING_SEQ=$((REMAINING_SEQ - 1))
    done
    
    # Exit if all queries are answered
    if [[ ${#QUERIES[@]} == 0 ]]; then echo "All queries are solved" ; exit; fi

    #################################
    # Step 3: Multiquery
    REMAINING_TIME=$(echo "$TIMEOUT_TOTAL - $SECONDS"|bc)
    echo "--- Step 3: Multiquery processing" 
    echo "Remaining: ${#QUERIES[@]} queries and $REMAINING_TIME seconds" 
    
    # Join remaining query indexes in comma separated string
    MULTIQUERY_INPUT=$(sed -e "s/ /,/g" <<< ${QUERIES[@]})
    
    echo "Running multiquery on -x $MULTIQUERY_INPUT" 
    $TIMEOUT $TIMEOUT_TOTAL $VERIFYPN $STRATEGY_SEQ $OPTIONS -p $MODEL_PATH/model.pnml $MODEL_PATH/$CATEGORY -x $MULTIQUERY_INPUT 
    
    echo "End of script."
}

case "$BK_EXAMINATION" in

    StateSpace)
       echo        
       echo "*****************************************"
       echo "*  TAPAAL performing StateSpace search  *"
       echo "*****************************************"
        $TIMEOUT $TIMEOUT_TOTAL $VERIFYPN -n -p -q 0 -e $MODEL_PATH/model.pnml 
        ;;

        UpperBounds)	
       echo		
       echo "*****************************************"
       echo "*  TAPAAL CLASSIC verifying UpperBounds *"
       echo "*****************************************" 
        CATEGORY="UpperBounds.xml"
        STRATEGY_SEQ="-s BFS -r 2 -p -q 0"
        NUMBER=`cat $MODEL_PATH/$CATEGORY | grep "<property>" | wc -l`
        $TIMEOUT $TIMEOUT_TOTAL $VERIFYPN $STRATEGY_SEQ $OPTIONS $MODEL_PATH/model.pnml $MODEL_PATH/$CATEGORY -x "$(seq -s , 1 $NUMBER)" 
        ;;

    ReachabilityDeadlock)
       echo        
       echo "**********************************************"
       echo "*  TAPAAL checking for ReachabilityDeadlock  *"
       echo "**********************************************"
        STRATEGIES_PAR[0]="-s DFS --siphon-trap 30 -q 0"
        STRATEGIES_PAR[1]="-s BFS -q 0"
        STRATEGIES_PAR[2]="-s DFS -q 0"
        STRATEGIES_PAR[3]="-s RDFS -q 0"

        TIMEOUTS_PAR[0]=30
        TIMEOUTS_PAR[1]=$DEFAULT_TIMEOUT_PAR
        TIMEOUTS_PAR[2]=$DEFAULT_TIMEOUT_PAR
        TIMEOUTS_PAR[3]=$DEFAULT_TIMEOUT_PAR

        STRATEGY_SEQ="-s DFS -q 0"
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

        TIMEOUTS_PAR[0]=$DEFAULT_TIMEOUT_PAR
        TIMEOUTS_PAR[1]=$DEFAULT_TIMEOUT_PAR

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

        TIMEOUTS_PAR[0]=$DEFAULT_TIMEOUT_PAR
        TIMEOUTS_PAR[1]=$DEFAULT_TIMEOUT_PAR

        STRATEGY_SEQ="-s DFS"
        CATEGORY="CTLFireability.xml"
        verifyparallel
        ;;

    *)
        echo "DO_NOT_COMPETE"  
        exit 0
        ;;
esac



