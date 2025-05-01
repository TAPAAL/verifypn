For creating the VM remember:

- create tmp directory in ~mcc/BenchKit/bin/tmp and set the owner to mcc and
  group to users

- upload binary in BenchKit/bin and set the owner, group and executability

- upload the tapaah.sh script (and all xml files) into BenchKit/bin, set owner, group and executability

- upload the BenchKit_head.sh into ~mcc/BenchKit/ 

- install time, parallel (check for --will-cite) and bc

- to fix locales add to .profile and .bashrc (both in root and ~mcc directory): export PERL_BADLANG=0

- for running qemu, check the QEMU-README.txt file.
