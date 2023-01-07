#!/bin/sh
# test behavior when decrypting corrupt infile
#set -x
echo aaa > in.test.$$
/bin/rm -f out.test.$$
./test_cryptocopy -d -C aes -p asdfasdf in.test.$$ out.test.$$
retval=$?
if test $retval != 22 ; then
	echo test_cryptocopy returned wrong error: $retval
	rm -f in.test.$$ out.test.$$
	exit 1
else
	echo test_cryptocopy returned correct error
	rm -f in.test.$$ out.test.$$
	exit 0
fi
