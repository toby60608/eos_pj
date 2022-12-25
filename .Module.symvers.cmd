cmd_/home/ubuntu/eos_pj/Module.symvers := sed 's/\.ko$$/\.o/' /home/ubuntu/eos_pj/modules.order | scripts/mod/modpost -m -a  -o /home/ubuntu/eos_pj/Module.symvers -e -i Module.symvers   -T -
