
CFLAGS = -g -DZSOCK -I.

all: ftp

OBJS = cmd.o ftp.o main.o makeargs.o net_zsock.o

ftp: $(OBJS)
	$(CC) -o ftp $(OBJS) -L../kernel -lzsock -lutil


clean:
	rm -f *.o ftp *~
