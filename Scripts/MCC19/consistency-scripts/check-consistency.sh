cat $1 $2 | grep "FORMULA " | grep -v CANNOT | awk '{print $2 " " $3}' | sort | uniq | awk '{print $1}' | uniq -c | grep " 2" 
 

