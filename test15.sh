#!/bin/sh
# test behavior when infile is symlink to outfile
#set -x
touch out.test.$$
ln -s out.test.$$ in.test.$$
./test_cryptocopy -c in.test.$$ out.test.$$
retval=$?
rm in.test.$$ out.test.$$
if test $retval != 22; then
	echo test_cryptocopy returned wrong error: $retval
	exit 1
fi
echo test_cryptocopy failed with correct error
exit 0
