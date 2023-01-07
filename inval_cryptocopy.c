// SPDX-License-Identifier: GPL-2.0
#include <stdio.h>
#include <asm/unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <linux/limits.h>

// this program is used to pass invalid arguments for sys_cryptocopy
// which cannot be passed used the test_cryptocopy.c program

#ifndef __NR_cryptocopy
#error cryptocopy system call not defined
#endif

#define NUM_TESTS 7
#define KEYLEN 32

char *in = "in.txt";
char *out = "out.txt";
int keylen = KEYLEN;
char keybuf[KEYLEN + 1];

int bad_keybuf_pointer(void)
{
	return syscall(__NR_cryptocopy, in, out, NULL, keylen, 1);
}

int long_filename(void)
{
	char *longname = malloc(PATH_MAX + 5);

	for (int i = 0; i < PATH_MAX + 4; i++)
		longname[i] = 'a';
	longname[PATH_MAX + 4] = '\0';
	return syscall(__NR_cryptocopy, in, longname, keybuf, keylen, 1);
}

int unused_flags(void)
{
	return syscall(__NR_cryptocopy, in, out, keybuf, keylen,
		       1 | 16 | 32 | 64);
}

int short_key(void)
{
	return syscall(__NR_cryptocopy, in, out, keybuf, 8, 1);
}

int inval_flags(void)
{
	return syscall(__NR_cryptocopy, in, out, keybuf, keylen, 1 & 2);
}

int bad_infile_pointer(void)
{
	return syscall(__NR_cryptocopy, NULL, out, keybuf, keylen, 1);
}

int bad_outfile_pointer(void)
{
	return syscall(__NR_cryptocopy, in, NULL, keybuf, keylen, 1);
}

typedef int (*test_func)(void);
test_func tests[NUM_TESTS] = { bad_keybuf_pointer, long_filename,
			       unused_flags,	   short_key,
			       inval_flags,	   bad_infile_pointer,
			       bad_outfile_pointer };

void help(void)
{
	printf("Usage: ./inval_cryptocopy TEST_NUM\n");
}

int main(int argc, const char *argv[])
{
	int ret;

	if (argc != 2) {
		help();
		return -1;
	}
	ret = atoi(argv[1]);
	if (ret < 1 || ret > NUM_TESTS) {
		help();
		return -1;
	}
	for (int i = 0; i < keylen; i++)
		keybuf[i] = 'A' + i;
	ret = tests[ret - 1]();
	if (ret == 0)
		return 0;
	return errno;
}
