install qemu

first execute:

./run.sh

qemu-system-x86_64 -hdb mcc2023-input.vmdk -smp 1 -k en-gb -m 2048 -drive file=tapaal-mcc2023.vmdk -net nic -net user,hostfwd=tcp::2222-:22

and then

./login.sh

ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no -p 2222 -i bk-private_key root@localhost

To finish, click on the qemu windows, and press

ctrl-alt-2   quit

or

ESC, 2, quit
