# To extract the resuls - run this in the main directory
#for f in */BK_RESULTS/OUTPUTS/*; do
#   cat $f >> 1
#done

for t in {Reachability,LTL,CTL}{Cardinality,Fireability} UpperBounds ; do 

echo
echo "Category : " $t

echo "ALL-TOTAL: \c" 
TOTAL_ALL=$(cat 1 | grep FORMULA_NAME | grep -v echo | awk '{print $2}' | grep $t | sort | uniq | wc | awk '{print $1}')
echo $TOTAL_ALL "\c"
echo "\t ALL-ANSWERED: \c " 
TOTAL_ANSWERED=$(cat 1 | grep FORMULA  | grep -v FORMULA_NAME | awk '{print $2, $3}' | grep $t | sort | uniq | wc | awk '{print $1}')
echo $TOTAL_ANSWERED "\c"
PROC=$(echo "scale=2;100*$TOTAL_ANSWERED/$TOTAL_ALL" | bc)
echo "\t ALL-PERCENTAGE: " $PROC

echo "PT-TOTAL:  \c" 
TOTAL_ALL=$(cat 1 | grep FORMULA_NAME | grep -v echo | awk '{print $2}' | grep $t | grep "PT-" | wc | sort | uniq | awk '{print $1}')
echo $TOTAL_ALL "\c"
echo "\t PT-ANSWERED:  \c " 
TOTAL_ANSWERED=$(cat 1 | grep FORMULA  | grep -v FORMULA_NAME | awk '{print $2, $3}' | grep $t | grep "PT-" | sort | uniq | wc | awk '{print $1}')
echo $TOTAL_ANSWERED "\c"
PROC=$(echo "scale=2;100*$TOTAL_ANSWERED/$TOTAL_ALL" | bc)
echo "\t PT-PERCENTAGE:  " $PROC

echo "COL-TOTAL:  \c" 
TOTAL_ALL=$(cat 1 | grep FORMULA_NAME | grep -v echo | awk '{print $2}' | grep $t | grep "COL-" | wc | sort | uniq | awk '{print $1}')
echo $TOTAL_ALL "\c"
echo "\t COL-ANSWERED:  \c " 
TOTAL_ANSWERED=$(cat 1 | grep FORMULA  | grep -v FORMULA_NAME | awk '{print $2, $3}' | grep $t | grep "COL-" | sort | uniq | wc | awk '{print $1}')
echo $TOTAL_ANSWERED "\c"
PROC=$(echo "scale=2;100*$TOTAL_ANSWERED/$TOTAL_ALL" | bc)
echo "\t COL-PERCENTAGE: " $PROC

done
