#!/bin/sh
# test errno when keylen is too short
#set -x
touch in.txt
touch out.txt
./inval_cryptocopy 4
retval=$?
rm in.txt out.txt
if test $retval != 22; then
	echo inval_cryptocopy returned wrong error: $retval
	exit 1
fi
echo inval_cryptocopy returned correct error
exit 0
