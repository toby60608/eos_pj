cmd_/home/pi/eos_pj/modules.order := {   echo /home/pi/eos_pj/driver.ko; :; } | awk '!x[$$0]++' - > /home/pi/eos_pj/modules.order
