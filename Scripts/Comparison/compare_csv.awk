#!/usr/bin/gawk -f

BEGIN {
	FS=",";
	corrects=0;
	incorrects=0;
	unique=0;
	split("0 1 10 11 12 13 14 15 2 3 4 5 6 7 8 9", res_vec, " ");
}

FNR==NR && $1!="Input" {
	#print NR;
	tmpFS="";
	if ($2=="UpperBounds")
		tmpFS=" ";
	split($3,vector,tmpFS);
	for (i=1;i<=length(vector);i++) {
		difference[$1][$2][i][vector[i]]++;
		key=vector[i];
		best=0;
		for (k in difference[$1][$2][i]) {
			if (difference[$1][$2][i][k] > best && k!="?") {
				best=difference[$1][$2][i][k];
				key=k
			}
		}
		results[$1][$2][i]=key;
	};
	next;
}

$1!="Input" {
	split($3,output," ");
	if (length(output) < 2)
		split($3,output,"");
	if (length(results[$1][$2]) > 0)
		for (i=1;i<=length(results[$1][$2]);i++)
			if (results[$1][$2][i]==output[i] && output[i]!="?") {
#				print $1, "query", res_vec[i], "is correct";
				corrects++;
			} else if (results[$1][$2][i]=="?")
				unique++;
			else if (output[i]!="?" && output[i]!="") {
				incorrects++;
				print $1, $2, "query", res_vec[i], ": YOURS:", output[i], " MCC17:", results[$1][$2][i];
			};
	#else     
	#	print "No results for", $1
}

END {
	print "Consistent results:", corrects;
	print "Inconsistent results:", incorrects;
	print "Unique results:", unique;
}
