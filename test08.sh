#!/bin/sh
# test infile is symlink
#set -x
echo dummy test > in.test.$$
ln -s in.test.$$ in.testlink.$$
/bin/rm -f out.test.$$
./test_cryptocopy -c in.testlink.$$ out.test.$$
retval=$?
if test $retval != 0 ; then
	echo test_cryptocopy failed with error: $retval
	rm -f in.test.$$ in.testlink.$$ out.test.$$
	exit 1
else
	echo test_cryptocopy program succeeded
fi
# now verify that the two files are the same
if cmp in.test.$$ out.test.$$ ; then
	echo "test_cryptocopy: input and output files contents are the same"
else
	echo "test_cryptocopy: input and output files contents DIFFER"
	rm -f in.test.$$ in.testlink.$$ out.test.$$
	exit 1
fi
rm -f in.test.$$ in.testlink.$$ out.test.$$
exit 0
