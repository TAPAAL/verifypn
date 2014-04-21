for D in $(find INPUTS -mindepth 1 -maxdepth 1 -type d) ; do
    echo;
    echo "####################################################################";
    echo $D ;
    cp BenchKit_head.sh $D ;
    cd $D ;
    export BK_EXAMINATION=StateSpace ;
    ./BenchKit_head.sh 
    export BK_EXAMINATION=ReachabilityComputeBounds ;
    ./BenchKit_head.sh 
    export BK_EXAMINATION=ReachabilityDeadlock ;
    ./BenchKit_head.sh 
    export BK_EXAMINATION=ReachabilityCardinality ;
    ./BenchKit_head.sh 
    export BK_EXAMINATION=ReachabilityFireability ;
    ./BenchKit_head.sh 
    export BK_EXAMINATION=ReachabilityFireabilitySimple ;
    ./BenchKit_head.sh 
    cd ../..
done
