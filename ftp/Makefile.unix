
CFLAGS = -g -DZSOCK -I. -I../include

all: ftp

OBJS = cmd.o ftp.o main.o makeargs.o net_zsock.o
DEV  = -L../devices/ppp -lppp

ftp: $(OBJS)
	$(CC) -o ftp $(OBJS) $(DEV) -lutil -L ../kernel -lzsock 


clean:
	rm -f *.o ftp *~
