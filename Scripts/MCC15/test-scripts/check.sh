#Call the script with one argument (filename of the output of all tools)

echo
echo "This script prints the name of queries where there was inconsistency."
echo "If the list is empty, all is fine. The check is done only for FORMULA,"
echo "not for state space exploration."
echo "The list of inconsitent queries: "
cat $1 | grep FORMULA | grep -v CANNOT | awk '{if ($3!="") {print $1,$2,$3}}' |\
         sort  | uniq | awk '{print $2}' | uniq -d
echo
echo
