#!/bin/sh
# test behavior when outfile has no write permissions
#set -x
if test $(id -u) -eq 0 ; then
	echo hello world > in.test
	su -c "./test23.sh" ubuntu
	retval=$?
	rm -f in.test
	exit $retval
fi
./test_cryptocopy -c in.test out.test.$$
retval=$?
if test $retval != 13 ; then
	echo test_cryptocopy returned wrong error: $retval
	exit 1
fi
echo test_cryptocopy returned correct error
exit 0
