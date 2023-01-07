// SPDX-License-Identifier: GPL-2.0
#include <asm/unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <string.h>

#ifndef __NR_cryptocopy
#error cryptocopy system call not defined
#endif

int main(int argc, const char *argv[])
{
	int rc;
	char *in = "in.txt";
	char *out = "out.txt";
	char *keybuf = "123456789012345678901234";

	rc = syscall(__NR_cryptocopy, in, out, keybuf, strlen(keybuf), 1);
	//rc = syscall(__NR_cryptocopy, out, in, keybuf, strlen(keybuf), 2);
	if (rc == 0)
		printf("syscall returned %d\n", rc);
	else
		printf("syscall returned %d (errno=%d)\n", rc, errno);

	exit(rc);
}
