#!/bin/sh
# test for correct behavior when infile is absolute path
#set -x
echo dummy test > in.test.$$
/bin/rm -f out.test.$$
./test_cryptocopy -c $PWD/in.test.$$ out.test.$$
retval=$?
if test $retval != 0 ; then
	echo test_cryptocopy failed with error: $retval
	rm -f in.test.$$ out.test.$$
	exit 1
else
	echo test_cryptocopy program succeeded
fi
# now verify that the two files are the same
if cmp in.test.$$ out.test.$$ ; then
	echo "test_cryptocopy: input and output files contents are the same"
	rm -f in.test.$$ out.test.$$
	exit 0
else
	echo "test_cryptocopy: input and output files contents DIFFER"
	rm -f in.test.$$ out.test.$$
	exit 1
fi
