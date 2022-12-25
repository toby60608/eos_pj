cmd_/home/ubuntu/eos_pj/modules.order := {   echo /home/ubuntu/eos_pj/driver.ko; :; } | awk '!x[$$0]++' - > /home/ubuntu/eos_pj/modules.order
