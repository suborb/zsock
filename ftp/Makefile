CC	 = zcc
TARGET   = +z88
CFLAGS	 = -vn -startup=5 -preserve -DZSOCK
INCLUDES = -I. 
LIBS   = -lnet -lz88 
LDFLAGS	 = -create-app  

SOURCES = ftp.c cmd.c makeargs.c net_zsock.c main.c

OBJS = $(SOURCES:.c=.o)


all: ftp.com

ftp.com: $(OBJS)
	$(CC) $(TARGET) $(CFLAGS) -Cz+z88shell -startup=5 -o $@ $(OBJS) $(LDFLAGS) $(LIBS)

%.o: %.c
	$(CC) $(TARGET) $(CFLAGS) $(CMDCFLAGS) $(INCLUDES) -c $<

clean:
	rm -f *.opt *.o ftp.com 
