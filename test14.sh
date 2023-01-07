#!/bin/sh
# test behavior when infile is same as outfile
#set -x
touch out.test.$$
./test_cryptocopy -c out.test.$$ out.test.$$
retval=$?
rm out.test.$$
if test $retval != 22; then
	echo test_cryptocopy returned wrong error: $retval
	exit 1
fi
echo test_cryptocopy failed with correct error
exit 0
