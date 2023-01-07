#!/bin/sh
# test encrypting and decrypting large file
#set -x
/bin/rm -f in.test.$$ out.test.$$
dd if=/dev/urandom of=in.test.$$ count=10000 bs=4095
./test_cryptocopy -e -C aes -p asdfasdf in.test.$$ out.test.$$
retval=$?
if test $retval != 0 ; then
	echo test_cryptocopy failed with error: $retval
	rm -f in.test.$$ out.test.$$
	exit 1
else
	echo test_cryptocopy program succeeded
fi
./test_cryptocopy -d -C aes -p asdfasdf out.test.$$ out2.test.$$
# now verify that the two files are the same
if cmp in.test.$$ out2.test.$$ ; then
	echo "test_cryptocopy: input and output files contents are the same"
	rm -f in.test.$$ out.test.$$ out2.test.$$
	exit 0
else
	echo "test_cryptocopy: input and output files contents DIFFER"
	rm -f in.test.$$ out.test.$$ out2.test.$$
	exit 1
fi

