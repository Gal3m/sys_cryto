#!/bin/sh
# test behavior when outfile is directory
#set -x
touch in.test.$$
mkdir out.test.$$
./test_cryptocopy -c in.test.$$ out.test.$$
retval=$?
rm -rf in.test.$$ out.test.$$
if test $retval != 21; then
	echo test_cryptocopy returned wrong error: $retval
	exit 1
fi
echo test_cryptocopy failed with correct error
exit 0
