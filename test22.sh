#!/bin/sh
# test behavior when infile has no read permissions
#set -x
if test $(id -u) -eq 0 ; then
	echo hello world > in.test
	chmod 333 in.test
	su -c "./test22.sh" ubuntu
	retval=$?
	rm -f in.test
	exit $retval
fi
./test_cryptocopy -c in.test /tmp/out.test.$$
retval=$?
if test $retval != 13 ; then
	echo test_cryptocopy returned wrong error: $retval
	rm /tmp/out.test.$$
	exit 1
fi
echo test_cryptocopy returned correct error
exit 0
