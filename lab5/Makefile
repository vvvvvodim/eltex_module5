obj-m += netlink.o

all: kernel_module netlink_user

kernel_module:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

netlink_user: user.c netlink_common.h
	gcc -Wall -o user user.c

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f user