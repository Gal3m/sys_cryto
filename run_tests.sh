#!/bin/sh
for i in $(seq -w 01 26); do
	echo running test $i
	./test$i.sh 
	retval=$?
	if test $retval != 0 ; then
		echo "[FAILED]\n"
	else
		echo "[PASS]\n"
	fi
done
