CC	 = zcc
TARGET   = +z88
CFLAGS	 = -vn -DPLUGIN -startup=4
INCLUDES = -I.  -I../../kernel
LIBS	 = 
LDFLAGS	 = 


all:	slip.dev


slip.dev: slip_dev.c
	$(CC) $(TARGET) $(CFLAGS) $(INCLUDES) -o $@ $<


clean:
	rm -f *.opt *~ slip.dev zcc_opt.def
