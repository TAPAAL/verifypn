#!/bin/bash

# This is the initialization script for the participation of TAPAAL
# untimed engine verifypn in the Petri net competition 2018.
# BK_EXAMINATION: it is a string that identifies your "examination"
#UNCOMMENT FOR THE VIRTUAL MACHINE AND COMMENT OUT NEXT TWO LINES
MODEL_PATH=.
VERIFYPN=/home/mcc/BenchKit/bin/verifypn
TIMEOUT_TOTAL=$(echo "$BK_TIME_CONFINEMENT-10" | bc) 
TEMPDIR="/home/mcc/tmp"

#TEMPDIR="/scratch/srba/"
#TIMEOUT_TOTAL="3590"

TIMEOUT_CMD=timeout
TIME_CMD="/usr/bin/time -f \"@@@%e,%M@@@\" "

START_TIME=$(date +"%s")
SECONDS=0

#Allowed memory in kB
MEM="15000000"
ulimit -v $MEM

PAR_CMD=parallel

# Verification options
OPTIONS=""
echo "Total timeout: " "$TIMEOUT_TOTAL"

# Timeouts (in seconds)
TIMEOUT_SIMP=$(echo "$TIMEOUT_TOTAL/5" | bc)
TIMEOUT_LP=$(echo "$TIMEOUT_TOTAL/120" | bc)
TIMEOUT_RED=$(echo "$TIMEOUT_TOTAL/12" | bc)
TIMEOUT_PAR=$(echo "$TIMEOUT_TOTAL/14" | bc) # competition 4.29 min
TIMEOUT_SEQ_MIN=$(echo "$TIMEOUT_TOTAL/7" | bc) # competition 8.6 min
SIPHONTRAP=$(echo "$TIMEOUT_TOTAL/12" | bc)
SHORTRED=$(echo "$TIMEOUT_TOTAL/30" | bc)
EXTENDED=$(echo "$TIMEOUT_TOTAL/25" | bc)

STRATEGY_SEQ="-s RDFS -q 20 -l 5 -d $SHORTRED"
STRATEGY_MULTI="-s BFS -q 15 -l 3 -d $SHORTRED"

STRATEGIES_PAR[0]="-tar -s RDFS -q 0 -l 0 -d $SHORTRED"
STRATEGIES_PAR[1]="-s BestFS -q 0 -l 0 -d $SHORTRED"
STRATEGIES_PAR[2]="-s BFS -q 0 -l 0 -d $SHORTRED"
STRATEGIES_PAR[3]="-s DFS -q 0 -l 0 -d $SHORTRED"

STRATEGIES_RAND[0]="-s RDFS --seed-offset 0 -q $TIMEOUT_TOTAL -l 0 -d $SHORTRED"
STRATEGIES_RAND[1]="-s RDFS --seed-offset 1337 -q 0 -l 0 -d 0"
STRATEGIES_RAND[2]="-s RDFS --seed-offset 2018 -q 0 -l 0 -d $SHORTRED"
STRATEGIES_RAND[3]="-s RDFS --seed-offset 9220 -q 0 -l 0 -d $SHORTRED"

run_multi=false;

function time_left {
    T=$(date +"%s")
    SECONDS=$(echo "$T-$START_TIME" | bc)
    SECONDS=$(echo "$TIMEOUT_TOTAL-$SECONDS" | bc)
    echo "Time left:  " $SECONDS
    if [[ "$SECONDS" -le 0 ]] ; then
        echo "Out of time, terminating!"
        rm $QF
        rm $MF
        exit
    fi
}

time_left

function verifyparallel {
    # Keep track of time passed (in seconds)
    mkdir -p $TEMPDIR
    QF=$(mktemp --tmpdir=$TEMPDIR )
    MF=$(mktemp --tmpdir=$TEMPDIR )
    echo $TEMPDIR
    echo $QF
    echo $MF
    local NUMBER=`cat $MODEL_PATH/$CATEGORY | grep "<property>" | wc -l`
    QUERIES=( $(seq 1 $NUMBER) )
    #MULTIQUERY_INPUT=$(sed -e "s/ /,/g" <<< ${QUERIES[@]})
    MULTIQUERY_INPUT=$(echo ${QUERIES[@]} | sed -e "s/ /,/g")
    time_left

    # Step -1: Colored Overapproximation
    echo "---------------------------------------------------"
    echo "            Step -1: Stripping Colors              "
    echo "---------------------------------------------------"
    echo "Verifying stripped models ($NUMBER in total)        "
    TMP=$($TIMEOUT_CMD $TIMEOUT_SIMP $VERIFYPN -n -c -q $TIMEOUT_SIMP -l $TIMEOUT_LP -d $TIMEOUT_RED -z 4 -x $MULTIQUERY_INPUT $MODEL_PATH/model.pnml $MODEL_PATH/$CATEGORY )
    CNT=0
    SOLVED=$(echo "$TMP" | grep "FORMULA" | grep -oP "(?<=-)[0-9]+(?=( TRUE)|( FALSE)|( [0-9]))")

    for i in $SOLVED ; do 
        i=$(echo $i | sed -e "s/0*//")
        if [ -z "$i" ] ; then i="0"; fi
        echo "Solution found by stripping colors (step -1) for query index " $i
        unset QUERIES[$i]
        CNT=$(echo "$CNT+1" | bc)
    done
    NUMBER=$(echo "$NUMBER-$CNT" | bc)

    echo "$TMP"
    
    #MULTIQUERY_INPUT=$(sed -e "s/ /,/g" <<< ${QUERIES[@]})
    MULTIQUERY_INPUT=$(echo ${QUERIES[@]} | sed -e "s/ /,/g")
    time_left

    if [ -z "$MULTIQUERY_INPUT" ]; then echo "All queries are solved" ; time_left; rm $QF; rm $MF; exit; fi

    # Step 0: Simplification 
    echo "---------------------------------------------------"
    echo "            Step 0: Parallel Simplification        "
    echo "---------------------------------------------------"
    echo "Doing parallel simplification ($NUMBER in total)"
    echo "Total simplification timout is $TIMEOUT_SIMP -- reduction timeout is $TIMEOUT_RED"

    echo "$VERIFYPN -n -q $TIMEOUT_SIMP -l $TIMEOUT_LP -d $TIMEOUT_RED -z 4 -s OverApprox --write-simplified $QF --write-reduced $MF -x $MULTIQUERY_INPUT $MODEL_PATH/model.pnml $MODEL_PATH/$CATEGORY"

    TMP=$($TIMEOUT_CMD $SECONDS $VERIFYPN -n -q $TIMEOUT_SIMP -l $TIMEOUT_LP -d $TIMEOUT_RED -z 4 -s OverApprox --binary-query-io 2 --write-simplified $QF --write-reduced $MF -x $MULTIQUERY_INPUT $MODEL_PATH/model.pnml $MODEL_PATH/$CATEGORY)

    echo "$TMP"
    TMP=$(echo "$TMP" | grep "FORMULA" | wc -l)
    for i in $(seq 1 $TMP); do
      echo "Solution found by parallel simplification (step 0)"
    done

    local NUMBER=`echo "$NUMBER-$TMP" | bc`
    QUERIES=( $(seq 1 $NUMBER) )
    time_left
    echo
    if [ ! -s $MF ]; then
      echo "Model file after phase 0 is empty (CPN unfolding failed), exiting ..."
      rm $QF
      rm $MF
      exit
    fi
 
    # Step 1: Parallel
    echo "---------------------------------------------------"
    echo "            Step 1: Parallel processing            "
    echo "---------------------------------------------------"
    echo "Doing parallel verification of individual queries ($NUMBER in total)"
    echo "Each query is verified by ${#STRATEGIES_PAR[@]} parallel strategies for $TIMEOUT_PAR seconds"
    
    i=0
    for Q in ${QUERIES[@]}; do
    
        TIMEOUT_PAR=$(( $TIMEOUT_PAR < $SECONDS ? $TIMEOUT_PAR : $SECONDS))
        if [[ "$TIMEOUT_PAR" -le 0 ]] ; then echo "Out of time, terminating!"; time_left; rm $QF; rm $MF; exit; fi
        echo "------------------- QUERY ${Q} ----------------------"
        # Execute verifypn on all parallel strategies
        # All processes are killed if one process provides an answer 
        step1="$($PAR_CMD --line-buffer --halt now,success=1 --timeout $TIMEOUT_PAR --xapply\
            eval $TIME_CMD $VERIFYPN -n $OPTIONS {} $MF $QF --binary-query-io 1 -x $Q -n \
            ::: "${STRATEGIES_PAR[@]}" 2>&1)"

        if [[ $? == 0 ]]; then
            #unset QUERIES[$Q-1]
            QUERIES=(${QUERIES[@]:0:$i} ${QUERIES[@]:$(($i + 1))})
            i=$(echo "$i - 1" | bc)
            echo "Solution found by parallel processing (step 1)"
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
        time_left
        i=$(echo "$i + 1" | bc)
    done

    # Exit if all queries are answered
    if [[ ${#QUERIES[@]} == 0 ]]; then echo "All queries are solved" ; time_left; rm $QF; rm $MF; exit; fi
    

    # Step 2: Sequential
    echo "---------------------------------------------------"
    echo "           Step 2: Sequential processing           "
    echo "---------------------------------------------------"
    echo "Remaining ${#QUERIES[@]} queries are verified sequentially."
    echo "Each query is verified for a dynamic timeout (at least $TIMEOUT_SEQ_MIN seconds)"
    
    time_left
    # Count the number of remaining queries to try solving sequentially
    REMAINING_SEQ=${#QUERIES[@]}
    if [[ "$TIMEOUT_SEQ_MIN" -ne "0" ]] ; then
    i=0
    for Q in ${QUERIES[@]}; do
        echo "------------------- QUERY ${Q} ----------------------"
        # Calculate remaining time
        TIMEOUT_SEQ=$(echo "$SECONDS / $REMAINING_SEQ" | bc)
        if [[ "$TIMEOUT_SEQ_MIN" -gt "$TIMEOUT_SEQ" ]]; then TIMEOUT_SEQ=$TIMEOUT_SEQ_MIN; fi
        if [[ "$TIMEOUT_SEQ" -gt "$SECONDS" ]]; then TIMEOUT_SEQ=$SECONDS; break; fi
        if [[ "$TIMEOUT_SEQ" -le 0 ]] ; then echo "Out of time, terminating!"; time_left; rm $QF; rm $MF; exit; fi 

        # Execute verifypn on sequential strategy
        echo "Running query $Q for $TIMEOUT_SEQ seconds. Remaining: $REMAINING_SEQ queries and $SECONDS seconds"
        $TIME_CMD $TIMEOUT_CMD $TIMEOUT_SEQ $VERIFYPN -n $OPTIONS $STRATEGY_SEQ $MF $QF --binary-query-io 1 -n -x $Q
        RETVAL=$?

        if [[ $RETVAL == 0 ]]; then
            #unset QUERIES[$Q-1]
            QUERIES=(${QUERIES[@]:0:$i} ${QUERIES[@]:$(($i + 1))})
            i=$(echo "$i - 1" | bc)
            echo "Solution found by sequential processing (step 2)"
        else
            echo "No solution found"
        fi

        time_left
        REMAINING_SEQ=$((REMAINING_SEQ - 1))
        i=$(echo "$i + 1" | bc)
    done
    fi

    # Exit if all queries are answered
    if [[ ${#QUERIES[@]} == 0 ]]; then echo "All queries are solved" ; time_left; rm $QF; rm $MF; exit; fi

  if $run_multi; then 
    # Step 3: Multiquery
    time_left 
    echo "---------------------------------------------------"
    echo "           Step 3: Multiquery processing           "
    echo "---------------------------------------------------"
    echo "Remaining ${#QUERIES[@]} queries are solved using multiquery"
    echo "Time remaining: $SECONDS seconds of the initial $TIMEOUT_TOTAL seconds" 
    
    # Join remaining query indexes in comma separated string
    #MULTIQUERY_INPUT=$(sed -e "s/ /,/g" <<< ${QUERIES[@]})
    MULTIQUERY_INPUT=$(echo ${QUERIES[@]} | sed -e "s/ /,/g")
    
    RED=$(echo "$SECONDS/8" | bc)
    RUN_TIME=$(echo "$SECONDS*6/8" | bc)
    if [[ "$RUN_TIME" -le 0 ]] ; then echo "Out of time, terminating!"; time_left; rm $QF; rm $MF; exit; fi
    echo "Running multiquery on -x $MULTIQUERY_INPUT for $RUN_TIME seconds" 
    TMP=$($TIME_CMD $TIMEOUT_CMD $RUN_TIME $VERIFYPN -n $STRATEGY_MULTI $OPTIONS -d $RED -q $RED -p $MF $QF --binary-query-io 1 -n -x $MULTIQUERY_INPUT )

    echo "$TMP"

    SOLVED=$(echo "$TMP" | grep -oP "(?<=Query index )[0-9]+(?= was solved)")
    SOLVED=$(echo $SOLVED | tr " " "\n" | sort -g -r)

    for rem in $SOLVED ; do 
            QUERIES=(${QUERIES[@]:0:$rem} ${QUERIES[@]:$(($rem + 1))})
            echo "Solution found by multiquery processing (step 3) for query index" $rem 
    done
  fi


    for trial in $(seq 0 20); do
        time_left
        if [[ ${#QUERIES[@]} == 0 ]]; then echo "All queries are solved" ; time_left; rm $QF; rm $MF; exit; fi
        # Step 4: Parallel random search
        echo "---------------------------------------------------"
        echo "            Step 4: Random Parallel processing     "
        echo "---------------------------------------------------"
        echo "Doing random parallel verification of individual queries (${#QUERIES[@]}  in total)"
        RUN_TIME=$(echo "$SECONDS/${#QUERIES[@]}" | bc)
        echo "Each query is verified by ${#STRATEGIES_RAND[@]} parallel strategies for $RUN_TIME seconds"
        
        i=0 
        for Q in ${QUERIES[@]}; do
            echo "------------------- QUERY ${Q} ----------------------"
            # Execute verifypn on all parallel strategies
            # All processes are killed if one process provides an answer 
            RUN_TIME=$(( $RUN_TIME < $SECONDS ? $RUN_TIME : $SECONDS))
            if [[ "$RUN_TIME" -le 0 ]] ; then echo "Out of time, terminating!"; time_left; rm $QF; rm $MF; exit; fi
            step1="$($PAR_CMD --line-buffer --halt now,success=1 --timeout $RUN_TIME --xapply\
                eval $TIME_CMD $VERIFYPN -n $OPTIONS {} $MF $QF --binary-query-io 1 -x $Q -n \
                ::: "${STRATEGIES_RAND[@]}" 2>&1)"

            if [[ $? == 0 ]]; then
                #unset QUERIES[$Q-1]
                QUERIES=(${QUERIES[@]:0:$i} ${QUERIES[@]:$(($i + 1))})
                i=$(echo "$i - 1" | bc)
                echo "Solution found in random processing (step 4)"
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
            time_left
            i=$(echo "$i + 1" | bc)
        done
    # Exit if all queries are answered
    done
 
    time_left
    echo "End of script."
    rm $QF
    rm $MF
}

case "$BK_EXAMINATION" in

    StateSpace)
        echo        
        echo "*****************************************"
        echo "*  TAPAAL performing StateSpace search  *"
        echo "*****************************************"
        $TIME_CMD $TIMEOUT_CMD $TIMEOUT_TOTAL $VERIFYPN -n -p -q 0 -e -s BFS $MODEL_PATH/model.pnml 
        time_left
        ;;

    UpperBounds)    
        echo        
        echo "*****************************************"
        echo "*  TAPAAL CLASSIC verifying UpperBounds *"
        echo "*****************************************" 
        #STRATEGIES_PAR[0]="-s BestFS -q 0 -l 0 -d $SHORTRED"
        #STRATEGIES_PAR[1]="-s BestFS -q 0 -l 0 -d 0"
        #STRATEGIES_PAR[0]="-s BFS -q 0 -l 0 -d $SHORTRED"
        STRATEGIES_PAR[0]="-n -s BFS -q 0 -l 0 -d $SHORTRED"
        STRATEGIES_PAR[1]="-n -p -q 0 -l 0 -d $SHORTRED"
        STRATEGIES_PAR[2]="-n -p -s DFS -q 0 -l 0 -d $SHORTRED"
        #STRATEGIES_PAR[2]="-n -tar -q 0 -l 0 -d 0"
        #unset STRATEGIES_PAR[2]
        unset STRATEGIES_PAR[3]
        CATEGORY="UpperBounds.xml"
        TIMEOUT_SEQ_MIN=0 
        TIMEOUT_PAR=$(echo "$TIMEOUT_TOTAL/26" | bc) # competition 2.3 min
        run_multi=true
        verifyparallel 
        ;;

    GlobalProperties)
        echo        
        echo "**********************************************"
        echo "*  TAPAAL checking for GlobalProperties       *"
        echo "**********************************************"
        STRATEGIES_PAR[0]="-tar --siphon-trap $SIPHONTRAP"
        STRATEGIES_PAR[1]="-s BFS -q 0 -l 0 -d $EXTENDED"
        STRATEGIES_PAR[2]="-s DFS -q 0 -l 0 -d $EXTENDED"
        STRATEGIES_PAR[3]="-s RDFS -q 0 -l 0 -d $EXTENDED"
        TIMEOUT_SEQ_MIN=0 
        CATEGORY="GlobalProperties.xml"
        TIMEOUT_PAR=$(echo "$TIMEOUT_TOTAL/6" | bc) # competition 10 min
        verifyparallel 
        ;;

    ReachabilityCardinality)
        echo        
        echo "**********************************************"
        echo "*  TAPAAL verifying ReachabilityCardinality  *"
        echo "**********************************************"
        CATEGORY="ReachabilityCardinality.xml"
        verifyparallel 
        ;;

    ReachabilityFireability)
        echo        
        echo "**********************************************"
        echo "*  TAPAAL verifying ReachabilityFireability  *"
        echo "**********************************************"
        CATEGORY="ReachabilityFireability.xml"
        TIMEOUT_SEQ_MIN=$(echo "$TIMEOUT_TOTAL/6" | bc) # competition 10 min
        verifyparallel 
        ;;

    CTLCardinality)
        echo        
        echo "*************************************"
        echo "*  TAPAAL verifying CTLCardinality  *"
        echo "*************************************"
        CATEGORY="CTLCardinality.xml"
        #unset STRATEGIES_PAR[3]
        TIMEOUT_PAR=$(echo "$TIMEOUT_TOTAL/12" | bc) # competition 5 min
        verifyparallel
        ;;

    CTLFireability)
        echo        
        echo "*************************************"
        echo "*  TAPAAL verifying CTLFireability  *"
        echo "*************************************"
        CATEGORY="CTLFireability.xml"
        #unset STRATEGIES_PAR[3]
        TIMEOUT_PAR=$(echo "$TIMEOUT_TOTAL/12" | bc) # competition 5 min
        TIMEOUT_SEQ_MIN=$(echo "$TIMEOUT_TOTAL/6" | bc) # competition 10 min
        verifyparallel
        ;;

    *)
        echo "DO_NOT_COMPETE"  
        exit 0
        ;;
esac
