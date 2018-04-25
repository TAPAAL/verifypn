qemu-system-x86_64 -curses -hdb mcc2018-input.vmdk -smp 1 -k en-gb -m 2048 -drive file=tapaal-mcc2018.vmdk -net nic -net user,hostfwd=tcp::2222-:22
#qemu-system-x86_64 -vnc :42 -hdb mcc2018-input.vmdk -smp 1  -m 2048 -drive file=mcc2018.vmdk 
#-netdev hostfwd=tcp:2222::22

