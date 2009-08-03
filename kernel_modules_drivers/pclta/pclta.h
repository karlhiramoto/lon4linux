#ifndef _PCLTA_
	#define _PCLTA_

//----------------------------------------------------------------------------
// communication protocol

// network command and network buffer masks
#define NI_Q_CMD                0xf0
#define NI_QUEUE                0x0f

// output buffer required commands
#define niCOMM                  0x10
#define niNETMGMT               0x20
#define niSERVICE               0xE6

// output buffer NOT required commands
#define niRESET				0x50
#define niFLUSH_CANCEL		0x60
#define niFLUSH_COMPLETE	0x60
#define niONLINE			0x70
#define niOFFLINE			0x80
#define niFLUSH				0x90
#define niFLUSH_IGN			0xA0
#define niSLEEP				0xB0
#define niSSTATUS			0xE0
#define niIRQENA			0xE5

// output queues
#define niTQ				0x02
#define niTQ_P				0x03
#define niNTQ				0x04
#define niNTQ_P				0x05
#define niPRIORITY			0x01

// input queues
#define niRESPONSE			0x06
#define niINCOMING			0x08


//----------------------------------------------------------------------------
// insmod command errors

#define PCLTA_IOPERM      1        /* I/O ports permission denied */
#define PCLTA_HSHK        2        /* hardware handshake failed */
#define PCLTA_LRESET      3        /* node did'n leave RESET state */
#define PCLTA_RESP        4        /* no response after reset */
#define PCLTA_IRQPERM     5        /* IRQ permission denied */
#define PCLTA_STAT        6        /* no response on status message */
#define PCLTA_TSCV        7        /* transciever not supported */
#define PCLTA_MAJOR       8        /* major number permission denied */
#define PCLTA_CDEV        9        /* create device failure */


#endif //__PCLTA
