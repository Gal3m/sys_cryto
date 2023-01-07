#!/bin/sh
# test for correctly following umask
#set -x
echo dummy test > in.test.$$
/bin/rm -f out.test.$$
chmod 777 in.test.$$
umask 777
./test_cryptocopy -c in.test.$$ out.test.$$
retval=$?
if test $retval != 0 ; then
	echo test_cryptocopy failed with error: $retval
	rm -f in.test.$$ out.test.$$
	exit 1
else
	echo test_cryptocopy program succeeded
fi
# now verify that the permissions are correct
if test $(stat --printf="%a" out.test.$$) -eq 0 ; then
	echo "test_cryptocopy: output file has correct umask permissions"
	rm -f in.test.$$ out.test.$$
	exit 0
else
	echo "test_cryptocopy: output file has wrong permissions"
	rm -f in.test.$$ out.test.$$
	exit 1
fi
