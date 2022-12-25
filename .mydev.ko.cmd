cmd_/home/pi/eos_pj/mydev.ko := ld -r -EL  -maarch64elf -z noexecstack --build-id=sha1  -T scripts/module.lds -o /home/pi/eos_pj/mydev.ko /home/pi/eos_pj/mydev.o /home/pi/eos_pj/mydev.mod.o;  true
