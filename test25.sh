#!/bin/sh
# test encrypting and then decrypting with different key
#set -x
/bin/rm -f in.test.$$ out.test.$$
echo hello world > in.test.$$
./test_cryptocopy -e -C aes -p asdfasdf in.test.$$ out.test.$$
retval=$?
if test $retval != 0 ; then
	echo test_cryptocopy failed with error: $retval
	rm -f in.test.$$ out.test.$$
	exit 1
else
	echo test_cryptocopy encryption program succeeded
fi
./test_cryptocopy -d -C aes -p asdfasdfasdf out.test.$$ out2.test.$$
retval=$?
if test $retval = 13 ; then
	echo test_cryptocopy decryption failed with correct error
	rm -f in.test.$$ out.test.$$
	exit 0
else
	echo test_cryptocopy returned incorrect error: $retval
	rm -f in.test.$$ out.test.$$
	exit 1
fi
