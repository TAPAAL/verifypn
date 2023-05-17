#!/bin/bash

DIR=$(dirname "${BASH_SOURCE[0]}")

if [ -z "$MODEL_PATH" ] ; then
	MODEL_PATH=.
	echo "Setting MODEL_PATH=$MODEL_PATH"
else
	echo "Got MODEL_PATH=$MODEL_PATH"
fi

if [ -z "$VERIFYPN" ] ; then
	VERIFYPN="$DIR"/verifypn
	echo "Setting VERIFYPN=$VERIFYPN"
else
	echo "Got VERIFYPN=$VERIFYPN"
fi

TIMEOUT_TOTAL=$(echo "$BK_TIME_CONFINEMENT-10" | bc)

if [ -z "$TEMPDIR" ]; then
	TEMPDIR="$DIR/tmp"
	echo "Setting TEMPDIR=$TEMPDIR"
else
	echo "Got TEMPDIR=$TEMPDIR"
fi

TIMEOUT_CMD=timeout
TIME_CMD="/usr/bin/time -f \"@@@%e,%M@@@\" "

START_TIME=$(date +"%s")
SECONDS=0

#Allowed memory in kB
if [ -z "$BK_MEMORY_CONFINEMENT" ] ; then
	BK_MEMORY_CONFINEMENT="16000"
	echo "Setting BK_MEMORY_CONFINEMENT=$BK_MEMORY_CONFINEMENT"
else
	echo "Got BK_MEMORY_CONFINEMENT=$BK_MEMORY_CONFINEMENT"
fi
MEM=$(echo "$BK_MEMORY_CONFINEMENT-500" | bc)
MEM=$(echo "$MEM*1024" | bc)
echo "Limiting to $MEM kB"
ulimit -v $MEM

echo "Total timeout: " "$TIMEOUT_TOTAL"

# Timeouts (in seconds)
TIMEOUT_SIMP=$(echo "$TIMEOUT_TOTAL/5" | bc)
TIMEOUT_LP=$(echo "$TIMEOUT_TOTAL/120" | bc)
TIMEOUT_RED=$(echo "$TIMEOUT_TOTAL/12" | bc)
TIMEOUT_SEQ_MIN=$(echo "5*60" | bc) # 5 min timeout

STRATEGIES_SEQ[0]="-s RANDOMWALK -q 0 -l 0"
CATEGORY="${MODEL_PATH}/${BK_EXAMINATION}.xml"

function time_left {
    T=$(date +"%s")
    SECONDS=$(echo "$T-$START_TIME" | bc)
    SECONDS=$(echo "$TIMEOUT_TOTAL-$SECONDS" | bc)
    echo "Time left:  " $SECONDS
    if [[ "$SECONDS" -le 0 ]] ; then
        echo "Out of time, terminating!"
        exit
    fi
}

time_left

function verifysequential {
    # Keep track of time passed (in seconds)
    mkdir -p $TEMPDIR
    export QF=$(mktemp --tmpdir=$TEMPDIR )
    export MF=$(mktemp --tmpdir=$TEMPDIR )
    if [[ -z "$QF" ]] ; then
	    echo "ERROR: Could not create temporary query file"
	    exit
    fi
    if [[ -z "$MF" ]] ; then
	    echo "ERROR: Could not create temporary model file"
	    exit
    fi

    echo "TEMPDIR=$TEMPDIR"
    echo "QF=$QF"
    echo "MF=$MF"
    trap "rm $QF $MF ; echo terminated-with-cleanup ; exit" 0 # we trap the to make sure we cleanup
    local NUMBER=`cat $CATEGORY | grep "<property>" | wc -l`
    QUERIES=( $(seq 1 $NUMBER) )
    MULTIQUERY_INPUT=$(echo ${QUERIES[@]} | sed -e "s/ /,/g")
    time_left

    # Step -1: Colored Overapproximation
    echo "---------------------------------------------------"
    echo "            Step -1: Stripping Colors              "
    echo "---------------------------------------------------"
    echo "Verifying stripped models ($NUMBER in total)        "
    TMP=$($TIMEOUT_CMD $TIMEOUT_SIMP $VERIFYPN -n -c -q $TIMEOUT_SIMP -l $TIMEOUT_LP -d $TIMEOUT_RED -z 4 -x $MULTIQUERY_INPUT $MODEL_PATH/model.pnml $CATEGORY )
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

    MULTIQUERY_INPUT=$(echo ${QUERIES[@]} | sed -e "s/ /,/g")
    time_left

    if [ -z "$MULTIQUERY_INPUT" ]; then echo "All queries are solved" ; time_left; exit; fi

    # Step 0: Simplification
    echo "---------------------------------------------------"
    echo "            Step 0: Parallel Simplification        "
    echo "---------------------------------------------------"
    echo "Doing parallel simplification ($NUMBER in total)"
    echo "Total simplification timout is $TIMEOUT_SIMP -- reduction timeout is $TIMEOUT_RED"

    PAR_SIMP_TIMEOUT=$SECONDS

    echo "$TIMEOUT_CMD $PAR_SIMP_TIMEOUT $VERIFYPN -n -q $TIMEOUT_SIMP -l $TIMEOUT_LP -d $TIMEOUT_RED -z 4 -s OverApprox --binary-query-io 2 --write-simplified $QF --write-reduced $MF -x $MULTIQUERY_INPUT $MODEL_PATH/model.pnml $CATEGORY"

    TMP=$($TIMEOUT_CMD $PAR_SIMP_TIMEOUT $VERIFYPN -n -q $TIMEOUT_SIMP -l $TIMEOUT_LP -d $TIMEOUT_RED -z 4 -s OverApprox --binary-query-io 2 --write-simplified $QF --write-reduced $MF -x $MULTIQUERY_INPUT $MODEL_PATH/model.pnml $CATEGORY 2>&1 )

    echo "$TMP"
    TMP=$(echo "$TMP" | grep "FORMULA" | wc -l)
    for i in $(seq 1 $TMP); do
      echo "Solution found by parallel simplification (step 0)"
    done

    local NUMBER=`echo "$NUMBER-$TMP" | bc`
    QUERIES=( $(seq 1 $NUMBER) )

    if [[ ! -s "$QF" ]]; then
        echo "No simplified files created. Constructing non simplified files."
        time_left

        echo "$VERIFYPN -n -q 0 -d 0 -z 4 --binary-query-io 2 --write-simplified $QF --write-reduced $MF -x $MULTIQUERY_INPUT $MODEL_PATH/model.pnml $CATEGORY"
	TMP=$($TIMEOUT_CMD $SECONDS $VERIFYPN -n -q 0 -d 0 -z 4 --binary-query-io 2 --write-simplified $QF --write-reduced $MF -x $MULTIQUERY_INPUT $MODEL_PATH/model.pnml $CATEGORY  2>&1 )
	echo "$TMP"
    	TMP=$(echo "$TMP" | grep "FORMULA" | wc -l)
    	for i in $(seq 1 $TMP); do
      		echo "Solution found by recovery phase (step 0)"
    	done

    	local NUMBER=`echo "$NUMBER-$TMP" | bc`
    	QUERIES=( $(seq 1 $NUMBER) )
    fi

    time_left
    echo
    if [ ! -s $MF ]; then
      echo "Model file after phase 0 is empty (CPN unfolding failed), exiting ..."
      exit
    fi

    # Exit if all queries are answered
    if [[ ${#QUERIES[@]} == 0 ]]; then echo "All queries are solved" ; time_left; exit; fi


    # Step 2: Sequential
    echo "---------------------------------------------------"
    echo "           Step 2: Sequential processing           "
    echo "---------------------------------------------------"
    echo "Remaining ${#QUERIES[@]} queries are verified sequentially."
    echo "Each query is verified for $TIMEOUT_SEQ_MIN seconds"

    time_left
    # Count the number of remaining queries to try solving sequentially
    REMAINING_SEQ=${#QUERIES[@]}
    if [[ "$TIMEOUT_SEQ_MIN" -ne "0" ]] ; then
    i=0
    for Q in ${QUERIES[@]}; do
        echo "------------------- QUERY ${Q} ----------------------"
        # Calculate remaining time
        TIMEOUT_SEQ=$TIMEOUT_SEQ_MIN
        TIMEOUT_SEQ=$(( $TIMEOUT_SEQ_MIN < $SECONDS ? $TIMEOUT_SEQ_MIN : $SECONDS))
        if [[ "$TIMEOUT_SEQ" -le 0 ]] ; then echo "Out of time, terminating!"; time_left; exit; fi

        # Execute verifypn on sequential strategy
        echo "Running query $Q for $TIMEOUT_SEQ seconds. Remaining: $REMAINING_SEQ queries and $SECONDS seconds"
        step2="$($TIMEOUT_CMD $TIMEOUT_SEQ $TIME_CMD \
                $VERIFYPN -n $MF $QF --binary-query-io 1 -x $Q -s RANDOMWALK -q 0 -l 0 2>&1)"
        RETVAL=$?

        if [[ $RETVAL == 0 ]]; then
            QUERIES=(${QUERIES[@]:0:$i} ${QUERIES[@]:$(($i + 1))})
            i=$(echo "$i - 1" | bc)
            echo "Solution found by sequential processing (step 2)"
        else
            echo "No solution found"
        fi
        FORMULA_RESULT="$(echo "$step2"|grep -m 1 FORMULA)"

        if [[ -n "$FORMULA_RESULT" ]]; then
            echo "$step2" | sed "/$FORMULA_RESULT/d"
        else
            echo "$step2"
        fi
        echo 
        echo "$FORMULA_RESULT"

        time_left
        REMAINING_SEQ=$((REMAINING_SEQ - 1))
        i=$(echo "$i + 1" | bc)
    done
    fi

    # Exit if all queries are answered
    if [[ ${#QUERIES[@]} == 0 ]]; then echo "All queries are solved" ; time_left; exit; fi

    time_left
    echo "End of script."
}

case "$BK_EXAMINATION" in

   ReachabilityCardinality)
        echo        
        echo "**********************************************"
        echo "*  TAPAAL verifying ReachabilityCardinality  *"
        echo "**********************************************"
        verifysequential
        ;;

    ReachabilityFireability)
        echo        
        echo "**********************************************"
        echo "*  TAPAAL verifying ReachabilityFireability  *"
        echo "**********************************************"
        verifysequential
        ;;

    *)
        echo "DO_NOT_COMPETE"
        exit 0
        ;;
esac
