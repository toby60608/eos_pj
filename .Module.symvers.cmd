cmd_/home/pi/eos_pj/Module.symvers := sed 's/\.ko$$/\.o/' /home/pi/eos_pj/modules.order | scripts/mod/modpost -m -a  -o /home/pi/eos_pj/Module.symvers -e -i Module.symvers   -T -
