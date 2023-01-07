// SPDX-License-Identifier: GPL-2.0
#include <asm/unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <openssl/evp.h>
#include <string.h>
#include "sys_cryptocopy.h"

#ifndef __NR_cryptocopy
#error cryptocopy system call not defined
#endif

// TODO print errors to stderr
void print_help(char *errstr, int err)
{
	if (errstr)
		printf("%s", errstr);
	printf("Usage: ./test_cryptocopy [-h] -[e|d|c] \
			-C cipher_name -p password infile outfile\n");
	exit(err);
}

// return len of stripped pw
int strip_pw(char *pw)
{
	int i = 0, j = 0;

	while (pw[j]) {
		if (pw[j] == '\n') {
			j++;
			continue;
		}
		pw[i++] = pw[j++];
	}
	pw[i] = pw[j];
	return i;
}

int main(int argc, char *const *argv)
{
	int flags = 0;
	char *ciphertype = NULL;
	char *pw = NULL;
	unsigned char *digest = NULL;
	char *infile = NULL;
	char *outfile = NULL;
	char *optstr = "edcC:p:h";
	int ret, pwlen;
	int mode = FLAG_ENCRYPT | FLAG_DECRYPT | FLAG_COPY;

	while ((ret = getopt(argc, argv, optstr)) > 0) {
		switch (ret) {
		case 'h':
			print_help(NULL, 0);
		case '?':
			print_help(NULL, EINVAL);
		case 'e':
			if (flags & mode)
				print_help("Must specify e/d/c just once\n",
					   EINVAL);
			flags |= FLAG_ENCRYPT;
			break;
		case 'd':
			if (flags & mode)
				print_help("Must specify e/d/c just once\n",
					   EINVAL);
			flags |= FLAG_DECRYPT;
			break;
		case 'c':
			if (flags & mode)
				print_help("Must specify e/d/c just once\n",
					   EINVAL);
			flags |= FLAG_COPY;
			break;
		case 'C':
			ciphertype = optarg;
			break;
		case 'p':
			pw = optarg;
		}
	}
	// confirm all options are parsed
	if (!(flags & mode))
		print_help("Must specify -e, -d, or -c\n", EINVAL);
	if (flags & FLAG_COPY) {
		if (ciphertype || pw) {
			print_help("-C and -p should not be specified \
						if just copying\n",
						EINVAL);
		}
	} else if (!ciphertype || !pw) {
		print_help("-C and -p must be specified if not copying\n",
			   EINVAL);
	}
	if (optind != argc - 2)
		print_help("Must specify input and output file\n", EINVAL);
	infile = argv[optind++];
	outfile = argv[optind++];
	// TODO what to do with ciphertype?
	if (flags & FLAG_ENCRYPT || flags & FLAG_DECRYPT) {
		if (strcmp(ciphertype, "aes")) {
			printf("Only aes encryption is supported\n");
			exit(ENOTSUP);
		}
		pwlen = strip_pw(pw);
		if (pwlen < 6) {
			printf("Password must be at least 6 characters long\n");
			exit(EINVAL);
		}
		// digest pw
		digest = malloc(32);
		if (!digest) {
			perror("Error");
			exit(ENOMEM);
		}
		ret = PKCS5_PBKDF2_HMAC(pw, pwlen, NULL, 0, 1000, EVP_sha256(),
					32, digest);
		if (!ret) {
			printf("PKCS failed\n");
			free(digest);
			exit(1);
		}
	}
	ret = syscall(__NR_cryptocopy, infile, outfile, digest, 32, flags);
	if (digest)
		free(digest);
	if (ret) {
		printf("syscall returned %d (errno=%d)\n", ret, errno);
		perror("Error");
	}
	exit(errno);
}
