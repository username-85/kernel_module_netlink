obj-m+=pblock_module.o

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
	$(CC) pblock.c -o pblock
clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
	rm pblock

