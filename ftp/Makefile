
CFLAGS = -g

all: z88mail

OBJS = cmd.o ftp.o main.o makeargs.o net_unix.o

z88mail: $(OBJS)
	$(CC) -o ftp $(OBJS)


clean:
	rm -f *.o ftp *~
