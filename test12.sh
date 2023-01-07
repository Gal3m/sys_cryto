#!/bin/sh
# test behavior when infile is directory
#set -x
./test_cryptocopy -c $PWD out.test.$$
retval=$?
if test $retval != 21 ; then
	echo test_cryptocopy returned wrong error: $retval
	exit 1
fi
echo test_cryptocopy returned correct error
exit 0
