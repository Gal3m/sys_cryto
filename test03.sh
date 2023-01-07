#!/bin/sh
# test behavior when unused flags are set
# program should safely ignore these flags
#set -x
touch in.txt
touch out.txt
./inval_cryptocopy 3
retval=$?
rm in.txt out.txt
if test $retval != 0; then
	echo inval_cryptocopy returned wrong error: $retval
	exit 1
fi
echo inval_cryptocopy returned correct error
exit 0
