
CFLAGS = -g

all: ftp

OBJS = cmd.o ftp.o main.o makeargs.o net_unix.o

ftp: $(OBJS)
	$(CC) -o ftp $(OBJS)


clean:
	rm -f *.o ftp *~
