#!/bin/sh
# test behavior when filename is too long
#set -x
touch in.txt
touch out.txt
./inval_cryptocopy 2
retval=$?
rm in.txt out.txt
if test $retval != 36; then
	echo inval_cryptocopy returned wrong error: $retval
	exit 1
fi
echo inval_cryptocopy returned correct error
exit 0
