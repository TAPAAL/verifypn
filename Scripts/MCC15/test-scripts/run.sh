    PREFIX=/Users/srba/dev/MCC-15/Testing/
    TOOL=BenchKit_head.sh
    INPUTSPATH=INPUTS

for D in $(find $INPUTSPATH -mindepth 1 -maxdepth 1 -type d) ; do
    echo;
    echo "####################################################################";
    echo $D ;
    cp BenchKit_head.sh $D ;
    cd $D ;

    for TOOLNAME in classicSEQ classicMC ontheflySEQ ontheflyMC ontheflyPAR; do
      export BK_TOOL=$TOOLNAME ;
      
      for EXAMINATION in StateSpace ReachabilityBounds \
                         ReachabilityComputeBounds ReachabilityDeadlock \
                         ReachabilityCardinality ReachabilityFireability \
                         ReachabilityFireabilitySimple; do
         export BK_EXAMINATION=$EXAMINATION ;
         $PREFIX$TOOL
      done 
    done 
    cd ../..
done
