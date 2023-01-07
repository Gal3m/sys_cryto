#!/bin/sh
# test errno when keybuf is invalid pointer
#set -x
touch in.txt
touch out.txt
./inval_cryptocopy 1
retval=$?
rm in.txt out.txt
if test $retval != 14; then
	echo inval_cryptocopy returned wrong error: $retval
	exit 1
fi
echo inval_cryptocopy returned correct error
exit 0
