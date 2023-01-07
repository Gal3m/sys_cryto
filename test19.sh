#!/bin/sh
# test for failure when outfile already exists
# but all tmp names are taken
#set -x
echo dummy test > in.test.$$
echo old outfile > out.test.$$
for x in $(seq 65 90); do
char=$(printf \\$(printf '%03o' $x))
touch out.test.$$$char
done
for x in $(seq 97 122); do
char=$(printf \\$(printf '%03o' $x))
touch out.test.$$$char
done
./test_cryptocopy -c in.test.$$ out.test.$$
retval=$?
for x in $(seq 65 90); do
char=$(printf \\$(printf '%03o' $x))
rm out.test.$$$char
done
for x in $(seq 97 122); do
char=$(printf \\$(printf '%03o' $x))
rm out.test.$$$char
done
if test $retval != 17 ; then
	echo test_cryptocopy failed with error: $retval
	rm -f in.test.$$ out.test.$$
	exit 1
else
	echo test_cryptocopy failed with correct error
	rm -f in.test.$$ out.test.$$
fi
exit 0
