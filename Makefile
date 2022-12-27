
export CROSS_COMPILE=aarch64-linux-gnu-
export ARCH=arm64
#sudo date  --s="2022-12-25 14:55:00"
#sudo timedatectl set-timezone Asia/Taipei

#obj-m += mydev.o
obj-m += driver.o
KDIR = ../linux
#KDIR = /lib/modules/$(shell uname -r)/build

all: server client driver

mydev: mydev.c project.h
	sudo make -C $(KDIR) M=$(shell pwd) CROSS_COMPILE=$(CROSS_COMPILE) ARCH=$(ARCH) modules

driver: driver.c
	sudo make -C $(KDIR) M=$(shell pwd) CROSS_COMPILE=$(CROSS_COMPILE) ARCH=$(ARCH) modules

writer: writer.c project.h
#	$(CROSS_COMPILE)gcc writer.c -o writer
	gcc writer.c -o writer

server: server.c SentryCamera/TestCamera.c project.h
	$(CROSS_COMPILE)gcc server.c SentryCamera/TestCamera.c -o server -lpthread
#	gcc server.c SentryCamera/TestCamera.c -o server -lpthread


client: client.c project.h
#	$(CROSS_COMPILE)gcc client.c -o client
	gcc client.c -o client

clean:
	make -C $(KDIR) M=$(shell pwd) clean
	@if [ -f "server" ]; then rm server ; fi
	@if [ -f "client" ]; then rm client ; fi
	@if [ -f "writer" ]; then rm writer ; fi


