PREFIX=$HOME/Datalogi/speciale/tools/flagship/Scripts/MCC17/test-scripts
TOOL=BenchKit_head.sh
INPUTSPATH=$HOME/Datalogi/speciale/models/small

for D in $(find $INPUTSPATH -mindepth 1 -maxdepth 1 -type d) ; do 
    echo;
    echo "####################################################################";
    echo "Model: $(basename $D)" ;
    cd $D
    for EXAMINATION in \
      ReachabilityCardinality\
      CTLCardinality\
      StateSpace\
      UpperBounds\
      ; do

      export BK_EXAMINATION=$EXAMINATION ;    
      for TOOLNAME in tapaal ; do
          export BK_TOOL=$TOOLNAME ;
          $PREFIX/$TOOL
      done 
    done 
    cd $PREFIX
done

