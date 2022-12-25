ccflags-y := -std=gnu99 -Wno-declaration-after-statement
MainFileName = TestCamera
FunctionFile1Name = SentryMode

#all: RPI_build
all: x86_build

clean:
ifeq ($(shell test -e $(MainFileName)  && echo -n true),true)
	sudo rm $(MainFileName)
endif
ifeq ($(shell test -e $(FunctionFile1Name)  && echo -n true),true)
	sudo rm $(FunctionFile1Name)
endif

x86_build: $(MainFileName).c $(FunctionFile1Name).c
	gcc -o $(MainFileName) $(MainFileName).c -I -std=gnu99
	gcc -o $(FunctionFile1Name) $(FunctionFile1Name).c -I -std=gnu99 -lpthread

RPI_build: $(MainFileName).c $(FunctionFile1Name).c
	aarch64-linux-gnu-gcc -o $(MainFileName) $(MainFileName).c -I -std=gnu99
	aarch64-linux-gnu-gcc -o $(FunctionFile1Name) $(FunctionFile1Name).c -I -std=gnu99 -lpthread

.PHONY: all clean x86_build x86_clean RPI_build RPI_clean
