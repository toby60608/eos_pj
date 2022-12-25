
#export CROSS_COMPILE=aarch64-linux-gnu-
#export ARCH=arm64
#sudo date  --s="2022-12-24 15:36:00"
#sudo timedatectl set-timezone Asia/Taipei

obj-m += mydev.o
KDIR = ../linux

all: server client

mydev: mydev.c project.h
	make -C $(KDIR) M=$(shell pwd) modules

writer: writer.c project.h
	$(CROSS_COMPILE)gcc writer.c -o writer

server: server.c project.h
	$(CROSS_COMPILE)gcc server.c -o server -lpthread
#	gcc server.c -o server


client: client.c project.h
	$(CROSS_COMPILE)gcc client.c -o client
#	gcc client.c -o client

clean:
	make -C $(KDIR) M=$(shell pwd) clean
	@if [ -f "server" ]; then rm server ; fi
	@if [ -f "client" ]; then rm client ; fi
	@if [ -f "writer" ]; then rm writer ; fi


