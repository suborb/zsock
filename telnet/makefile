#zcc -m -nt -lnet -lz88 -reqpag=10 -create-app -make-app telnet.c vt52_asm.c telnet_app.c tftpd.c tftpd-utils.c chat.c -lmalloc

zcc -m -nt -startup=5 -o telnet telnet_cmd.c vt52_asm.c -lmalloc -lnet -lz88
shellmak telnet telnet
