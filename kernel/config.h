#ifndef CONFIG_H
#define CONFIG_H

/*
 * Interrupts aren't working quite as they should be
 * so we have to busy loop, this file sets up various
 * things for this..
 */

#define BUSYLOOP() GoTCP()
#define BUSYINT()  Interrupt()

#define BUSY_VERSION 1

//#define HCALL (s->datahandler)

#define HCALL Handler_Call

#endif


