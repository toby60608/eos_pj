
export CROSS_COMPILE=aarch64-linux-gnu-
export ARCH=arm64

obj-m += mydev.o
KDIR = ../linux

all: server client mydev

mydev: mydev.c
	make -C $(KDIR) M=$(shell pwd) modules

server: server.c
	$(CROSS_COMPILE)gcc server.c -o server
#	gcc server.c -o server

client: client.c
	$(CROSS_COMPILE)gcc client.c -o client
#	gcc client.c -o client

clean:
	make -C $(KDIR) M=$(shell pwd) clean
	@if [ -f "server" ]; then rm server ; fi
	@if [ -f "client" ]; then rm client ; fi


