#ifndef SCCZ80

int ser_error = 0;
int sock = -1;

serial_init()
{
    printf("Initialising PPP\n");
    sock = open_terminal("ppp");
}

void serial_out(unsigned char ch)
{
    if ( write(sock,&ch,sizeof(unsigned char)) == -1 )
	printf("Write error\n");
}


int serial_in()
{
    unsigned char  ch[2];

    if ( read(sock,&ch,1) == -1 ) {
        return -1;
    }
    return ch[0];
}



#else
#ifdef __Z88__
#asm

	INCLUDE	"#serintfc.def"
	INCLUDE "#stdio.def"

	XDEF    _serial_out
	XDEF    _serial_in

._serial_out
	pop	bc
	pop	hl
	push	hl
	push	bc
	ld	a,l
        ld      l,si_pbt        ;reason code
        ld      bc,0            ;timeout
        call_oz(os_si)
        ret 

._serial_in
        ld      l,si_gbt
        ld      bc,0
        call_oz(os_si)
	ld	l,a
	ld	h,0
        ret

#endasm
#endif

#endif


