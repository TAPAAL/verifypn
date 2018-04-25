cd $1
i=0
N[0]="RC"
N[1]="RF"
N[2]="RD"
N[3]="CC"
N[4]="CF"
N[5]="UB"
echo
echo -e "\tS-1 \tS0 \tS1 \tS2 \tS3 \tS4 \tTotal"
echo "-------------------------------------------------------------"
for pattern in ReachabilityCardinality ReachabilityFireability ReachabilityDeadlock CTLCardinality CTLFireability UpperBounds; do
  #echo -e $pattern "\c"
  A=$(ls . | grep $pattern) 
  j=0
  if [ ${#A} -ge 1 ]; then 
  echo -e ${N[${i}]} "\c"
  for step in "(step -1)" "(step 0)" "(step 1)" "(step 2)" "(step 3)" "(step 4)"; do
    B[${j}]=$(cat $A | grep "$step" | wc -l)
    echo -e ' \t\c'
    echo -e ${B[${j}]} "\c"
    j=$(echo "$j + 1" | bc)
  done
  echo -e "\t\c" 
  echo -e $(echo "${B[0]}+${B[1]}+${B[2]}+${B[3]}+${B[4]}+${B[5]}" | bc) "\c"
  echo
  fi
  i=$(echo "$i + 1" | bc)
done
echo
