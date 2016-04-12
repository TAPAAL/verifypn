    PREFIX=/Users/srba/dev/MCC-16/
    TOOL=BenchKit_head.sh
    INPUTSPATH=INPUTS-one

for D in $(find $INPUTSPATH -mindepth 1 -maxdepth 1 -type d) ; do
    echo;
    echo "####################################################################";
    echo $D ;
    cp BenchKit_head.sh $D ;
    cd $D ;

      
      for EXAMINATION in \
        UpperBounds \
        ; do
#        ReachabilityCardinality \
#        ReachabilityFireability \
#        ReachabilityDeadlock \
#        CTLFireability \
#        CTLCardinality \
#        StateSpace \
     
	export BK_EXAMINATION=$EXAMINATION ;
	  for TOOLNAME in tapaalSEQ tapaalEXP ; do
      	  export BK_TOOL=$TOOLNAME ;
          $PREFIX$TOOL
      done 
    done 
    cd ../..
done
