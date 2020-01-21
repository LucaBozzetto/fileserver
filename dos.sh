#!/bin/bash
echo $1
for (( c=1; c<=$1; c++ ))
do
	if [ -z $3 ]
	then
		./client -f $2 &
	else
		./client -a $2 -f $3 &
	fi
	# sleep 2s
done
 
wait 
echo "All done"