#!/bin/sh
# test infile does not exist
#set -x
/bin/rm -f in.test.$$ out.test.$$
./test_cryptocopy -c in.test.$$ out.test.$$
retval=$?
if test $retval != 2 ; then
	echo test_cryptocopy failed with wrong error: $retval
	exit 1
fi
echo test_cryptocopy failed with correct error
exit 0
