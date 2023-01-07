obj-m += sys_cryptocopy.o

INC=/lib/modules/$(shell uname -r)/build/arch/x86/include

all: quick_test cryptocopy test_cryptocopy inval_cryptocopy

test_cryptocopy: test_cryptocopy.c
	gcc -Wall -Werror -I$(INC)/generated/uapi -I$(INC)/uapi test_cryptocopy.c -o test_cryptocopy -lcrypto

inval_cryptocopy: inval_cryptocopy.c
	gcc -Wall -Werror -I$(INC)/generated/uapi -I$(INC)/uapi inval_cryptocopy.c -o inval_cryptocopy

quick_test: quick_test.c
	gcc -Wall -Werror -I$(INC)/generated/uapi -I$(INC)/uapi quick_test.c -o quick_test

cryptocopy:
	#rmmod sys_cryptocopy 2>/dev/null || true
	make -Wall -Werror -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	#insmod sys_cryptocopy.ko

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f quick_test test_cryptocopy inval_cryptocopy
