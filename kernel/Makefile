TARGET   = +z88
#TARGET   = +cpm


CC	 = zcc
CFLAGS	 = -vn -DNETSTAT -DINACTIVETO=1200 -make-app -O3  -preserve
INCLUDES = -I. -I../include
LIBS	 = -lnetdev -lmalloc
LDFLAGS	 = 

SOURCES  := setup.c icmp.c ip.c udp.c tcp.c udp_dom.c  \
	handler_call.c net_utils.c loopback.c pbuf.c socket.c socket_int.c \
	pppset.c slip_dev.c z80.c time.c tcp_int.c ping.c lowlevel2.c device.c

ifeq (+z88,$(TARGET))
	SOURCES +=  config_read.c main.c app.c
	LDFLAGS = -create-app -nt
	LIBS += -lz88
endif

ifeq (+cpm,$(TARGET))
	SOURCES += unixmain.c
endif

ASMS	  := $(SOURCES:.c=.opt)

all:	$(ASMS)
	$(CC) $(TARGET) $(LDFLAGS) -o zsock $(CFLAGS) $(ASMS) $(LIBS)



%.opt: %.c zsock.h
	$(CC) $(TARGET) $(CFLAGS) -a $(INCLUDES)  $<


clean:
	rm -f *.opt *~ zsock zsock.62 zsock.63 zcc_opt.def
