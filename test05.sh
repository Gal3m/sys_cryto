#!/bin/sh
# test errno when invalid combination of flags is passed
#set -x
touch in.txt
touch out.txt
./inval_cryptocopy 5
retval=$?
rm in.txt out.txt
if test $retval != 22; then
	echo inval_cryptocopy returned wrong error: $retval
	exit 1
fi
echo inval_cryptocopy returned correct error
exit 0
