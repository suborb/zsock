CC	 = zcc
TARGET   = +z88
CFLAGS	 = -vn 
INCLUDES = -I. 
LIBS   = -lnet -lz88 -lmalloc
LDFLAGS	 = -create-app  
APPCFLAGS = -make-app -reqpag=10 -create-app -preserve
CMDCFLAGS = -startup=5 -preserve

APPSOURCES = telnet.c vt52_asm.c telnet_app.c tftpd.c tftpd-utils.c chat.c
CMDSOURCES = telnet_cmd.c vt52_asm.c

APPASMS = $(APPSOURCES:.c=.opt)
CMDOBJS = $(CMDSOURCES:.c=.o)


all: telnet.63 telnet.com

telnet.63: cleanzcc $(APPASMS)
	$(CC) $(TARGET) $(CFLAGS) $(APPCFLAGS) $(LDFLAGS) -o telnet $(APPASMS) $(LIBS)

telnet.com: cleanzcc $(CMDOBJS)
	$(CC) $(TARGET) $(CFLAGS) $(CMDCFLAGS) -Cz+z88shell -startup=5 -o $@ $(CMDOBJS) $(LDFLAGS) $(LIBS)

%.opt: %.c 
	$(CC) $(TARGET) $(CFLAGS) $(APPCFLAGS) $(INCLUDES) -a $<

%.o: %.c
	$(CC) $(TARGET) $(CFLAGS) $(CMDCFLAGS) $(INCLUDES) -c $<

.PHONY: cleanzcc

cleanzcc:
	rm -f zcc_opt.def

clean:
	rm -f *.opt *.o telnet.com telnet.bas telnet.63
