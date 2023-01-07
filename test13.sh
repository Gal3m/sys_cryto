#!/bin/sh
# test behavior when infile is not regular file (/dev/urandom)
#set -x
./test_cryptocopy -c /dev/urandom out.test.$$
retval=$?
if test $retval != 22; then
	echo test_cryptocopy returned wrong error: $retval
	exit 1
fi
echo test_cryptocopy failed with correct error
exit 0
