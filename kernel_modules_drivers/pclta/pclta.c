// pclta.c: Device driver for PCLTA LON-interface card 

//------------------------------------------------------------------------------
// version information
char * pclta_version = "00";
char * pclta_release = "2.4";

char * pclta_company = "Your Copany Inc.";
char * pclta_copyright = "1999-2001";

char * pclta_driver_name = "PCLTA";


//------------------------------------------------------------------------------
// include files


#include <linux/module.h>
//#include <linux/version.h>
//#include <linux/config.h>
    // include the configuration header file, to be certain that a module
    // is compiled together with the actual version of kernel
    // but, it seems not to work at the moment

#include <linux/kernel.h>		// for KERN_WARNING, ...
#include <asm/errno.h>				// error codes

#include <asm/uaccess.h>		// put_user()
#include <linux/poll.h>			// poll() function

#include <linux/slab.h>		// for kmalloc(), kfree(), ...
#include <linux/interrupt.h>
#include <linux/ioport.h>		// for *_region()
#include <asm/io.h>				// for in*(), out*()
#include <asm/irq.h>			// for disable_irq(), enable_irq()

#include "pclta.h"

MODULE_LICENSE("GPL");

//----------------------------------------------------------------------------
// messages



#define MESSAGE(fmt, arg... ) printk( "<1>\npclta: " fmt, ## arg )
#define VERBOSE_MSG(fmt, arg... ) if( pclta_verbose ) printk( "<1>pclta info: \n" fmt, ## arg )

#ifdef DEBUG
	#define DEBUG_MSG(fmt, arg... ) printk( "<1>\nPCLTA DEBUG: \n" fmt, ## arg )
	#define CALL_MSG(fmt, count, arg... ) printk( "<1>\npclta call:%d: " fmt, count, ## arg )
	#define RETURN_MSG(fmt, count, arg... ) printk( "<1>\npclta return:%d: " fmt, count, ## arg )
#else
	#define DEBUG_MSG(fmt, arg... )
	#define CALL_MSG(fmt, count, arg... )
	#define RETURN_MSG(fmt, count, arg... )
#endif //DEBUG

//------------------------------------------------------------------------------
// defines

// limits, buffer sizes, etc
#define MAX_DEVICES 8			// maximum number of devices
#define MAX_PACKET_SIZE 256		// maximum packet size (including command)
#define PACKETS_PER_BUFFER 16	// tx/rx buffer size
//#define PACKETS_PER_BUFFER 8	// tx/rx buffer size


//PCLTA hardware register offsets
#define data_reg(n)			(n)			// input/output
#define status_reg(n)		((n)+1)		// input
#define clrirq_reg(n)		((n)+1)		// output
#define selirq_reg(n)		((n)+2)		// output
#define clkrst_reg(n)		((n)+3)		// output


// register masks

// Status Register
#define SLEEPENG	0x20
#define DLPBA		0x10
#define DLBA		0x08
#define ULREADY		0x04
#define RESET		0x02
#define _READY		0x01

// Interrupt Clear Register
#define CLRIRQ		0x01

// IRQ Selection Register
#define IRQ15		0x20
#define IRQ12		0x10
#define IRQ11		0x08
#define IRQ10		0x04
#define IRQ9		0x02
#define IRQ5		0x01

// Clock and Reset Control Register
#define PCLTA_RESET			0x10
#define CLK_10				0x00
#define CLK_5				0x01
#define CLK_2_5				0x02
#define CLK_1_25			0x03
#define CLK_0_625			0x04

// Interrupt Reguest Control Register
#define IRQPLRTY			0x80
#define RESET_ENABLE		0x40
#define DLREADY_ENABLE		0x20
#define DLPBA_ENABLE		0x10
#define DLBA_ENABLE			0x08
#define ULREADY_ENABLE		0x04

// Transciever ID Register
#define TRANSCIEVER			0xf8
#define BATTERY				0x03 // only for PCNSS

// transfer control command bytes
#define CMD_XFER			0x01
#define CMD_ACK				0x02


//------------------------------------------------------------------------------
// type definitions

typedef enum { false = 0, true = 1 } bool;
typedef unsigned char byte;

enum device_state
{
	Idle,
	Offline,
	Transfer
};

//------------------------------------------------------------------------------
// data structure definitions


struct pclta_packet
{
 	struct pclta_packet * next;
	volatile bool busy;
	byte length;
	byte data[MAX_PACKET_SIZE];
};

struct pclta_packet_buffer
{
	struct pclta_packet * head;
	struct pclta_packet * tail;
	struct pclta_packet buffer[PACKETS_PER_BUFFER];
};

struct pclta_device
{
    unsigned int base_address;
    unsigned int interrupt;
    unsigned char txcvr_clock;
    int minor;
    struct wait_queue_head_t * pclta_wait_queue;
    struct tq_struct * interrupt_queue;
    bool restart_uplink;
    volatile enum device_state state;
    struct pclta_packet_buffer * rx;
    struct pclta_packet_buffer * tx;
};

//------------------------------------------------------------------------------
// variables

int pclta_major;	// device driver major number
char * pclta_device_name = "pclta";
volatile int pclta_call_level = 0;
//bool last_packet_read_complete=true;
//byte *current_read_ptr;
//byte bytes_left_to_read=0;

// input parameters for insmod
int base_address[MAX_DEVICES] = { 0x00, };
int pclta_interrupt[MAX_DEVICES] = { 0x00, };
int pclta_verbose = 0; // 0 = verbose, 1 = quiet

MODULE_PARM( base_address, "1-" __MODULE_STRING(MAX_DEVICES) "i" );
MODULE_PARM( pclta_interrupt, "1-" __MODULE_STRING(MAX_DEVICES) "i" );
MODULE_PARM( pclta_verbose, "1i" ); // 0 or 1

// device tables
struct pclta_device * pclta_device_table[MAX_DEVICES] = { NULL, };
struct pclta_device * pclta_interrupt_table[MAX_DEVICES] = { NULL, };


// timer handeler
void pclta_timer_handler( unsigned long data );

// timer list is defined in the kernel 
//struct timer_list pclta_timer_list = { NULL, NULL, 0, 0, timer_handler}; original karl removed 1 aug 2001

struct timer_list pclta_timer_list;

// timer list to activate timer

volatile int pclta_timeout;
    // variable to indicate if timeout occured

//------------------------------------------------------------------------------
// hardware-specific constants

// irq select register lookup table
const byte pclta_irq_mask[16] =
    { 0,0,0,0,0,IRQ5,0,0,0,IRQ9,IRQ10,IRQ11,IRQ12,0,0,IRQ15};

// supported transceiver types
// type code -to- string translation table
const char * pclta_transceiver[32] =
{
	"", "TP/XF-78", "", "TP/XF-1250",
	"FT-10", "TP-RS485-39", "", "RF-10",
	"", "PL-10", "TP-RS485-625", "TP-RS485-1250",
	"TP-RS485-78", "", "", "",
	"PL-20C", "PL-20N", "PL-30", "",
	"", "", "", "",
	"FO-10", "", "", "DC-78",
	"DC-625", "DC-1250", "", ""
};

// transceiver type -to- clock frequency translation table
const byte pclta_clock[32] =
{
	CLK_10, CLK_10, CLK_10, CLK_10,
	CLK_10, CLK_10, CLK_10, CLK_5,
	CLK_10, CLK_10, CLK_10, CLK_10,
	CLK_10, CLK_10, CLK_10, CLK_10,
	CLK_10, CLK_10, CLK_10, CLK_10,
	CLK_10, CLK_10, CLK_10, CLK_10,
	CLK_10, CLK_10, CLK_10, CLK_10,
	CLK_10, CLK_10, CLK_10, CLK_10,
};

//------------------------------------------------------------------------------
// prototypes

int init_module( void );
void cleanup_module( void );

bool probe_hardware( int minor );
void init_hw( int port, byte irq, byte txcvr_clock );
void cleanup_hw( struct pclta_device *dev );


//------------------------------------------------------------------------------
// registered functions

ssize_t read_pclta( struct file *file, char *buf, size_t cnt, loff_t *offset );

// junk
//long read_pclta(struct inode *dev_inode, struct file *dev_file, char *buffer, unsigned long count);
//long read_pclta(struct inode* dev_inode, struct file *dev_file, char *buffer, unsigned long count);

						
ssize_t write_pclta( struct file *file, const char *buf, size_t cnt, loff_t *offset );
			
unsigned int poll_pclta( struct file * file, struct poll_table_struct * poll );
		
int open_pclta( struct inode *inode, struct file *file );

int close_pclta( struct inode *inode, struct file *file );


// timer dependent read/write utilities

bool write_byte_timeout( int port, unsigned char byte,
    int sec100 );

bool wait_reset( unsigned int port, int sec100 );

bool wait_uplink_buffer( int port, int sec100, byte * buf );

void setup_timeout( int sec100 );

// -------------------------------
// callbacks for interrupt handler
// -------------------------------

void pclta_start_downlink( struct pclta_device *dev );
void pclta_service( struct pclta_device *dev );

// ------------------
// interrupt handlers
// ------------------

void pclta_interrupt_handler( int irq, void *dev_id, struct pt_regs *regs );
void timer_handler( unsigned long data );

// ----------------
// block read/write
// ----------------

bool uplink_packet( int port, byte * buffer );
void downlink_packet( int port, byte * buffer );

// ----------------------
// inline byte read/write
// ----------------------

inline unsigned char read_byte_wait( int port );
inline void write_byte_wait( int port, byte data_byte );

// --------------------------
// device structure functions
// --------------------------

struct pclta_device * create_device( int port, int irq, byte txcvr_clock );
	
void remove_device( struct pclta_device * device );
bool open_device( struct pclta_device * device );
void close_device( struct pclta_device * device );
void init_buffers( struct pclta_packet_buffer * buffer );


//------------------------------------------------------------------------------
// file operations table
/*  removed by karl aug 2001
static struct file_operations pclta_fops =
{
    NULL,              	// lseek()
    read_pclta,		// read()
    write_pclta,	// write()
    NULL,               // readdir()
    poll_pclta,		// poll()
    NULL,				// ioctl()
    NULL,				// mmap()
    open_pclta,		// open()
    NULL,				// flush()
    close_pclta,	// release()
    NULL,				// fsync()
    NULL,				// fasync()
    NULL,				// check_media_change()
    NULL,				// revalidate
    NULL				// lock
};
*/

// added by karl 1 aug 2001
static struct file_operations pclta_fops = {
   read: read_pclta,		// read()
   write: write_pclta,	// write()
   open: open_pclta,		// open()
   release: close_pclta,	// release()

};

//SET_MODULE_OWNER(&pclta_fops);  // added by karl 1 aug 2001

//////////////////TEST FUNCTION
void print_buffer( struct pclta_packet_buffer * packet_buffer )
{
	unsigned int i;
	
	CALL_MSG( "print_buffer()\n", ++pclta_call_level );

    DEBUG_MSG("Buffer  head= %ud tail= %ud\n",(unsigned int) packet_buffer->head,(unsigned int) packet_buffer->tail);
	for( i=0; i<PACKETS_PER_BUFFER; i++ ) {
	
	    DEBUG_MSG("PB: %d ; Busy= %d ; L= %d ; d0= %x ; d1= %x ; d2= %x \n",i ,packet_buffer->buffer[i].busy,packet_buffer->buffer[i].length,packet_buffer->buffer[i].data[0],packet_buffer->buffer[i].data[1],packet_buffer->buffer[i].data[2]);
	}
	// start at beginning of buffer

	RETURN_MSG( "print_buffer()", pclta_call_level-- );
}




//------------------------------------------------------------------------------
// Implementation

//-----------------------------------------------------------------
// init_module

int init_module( void )
{
	unsigned int i, count=0;

	printk( "\n%s driver for Linux\n  Version %s.%s", 
				pclta_driver_name, pclta_release, pclta_version );
				
	printk( "\n  Copyright %s, %s\n", pclta_copyright, pclta_company );

	// probe for devices specified in command line
	for( i=0; i<MAX_DEVICES; i++ )
	{
		if( probe_hardware( i ) )
			count++;
	}

	if( count ) {
		MESSAGE( "%d device(s) installed.\n", count );
		return 0;
	}
	else {
		MESSAGE( "No devices detected.\n" );
		return -ENXIO;
	}
}

//-----------------------------------------------------------------
// probe_hardware

bool probe_hardware( int minor )
{
    int erc, trans;
    byte rx_buffer[256];
    int port = base_address[minor];
    int irq  = pclta_interrupt[minor];
    struct pclta_device * device;

    if( !port || !irq )
    	return 0;

    VERBOSE_MSG( "probing minor = %d, I/O = %#x-%#x, interrupt request = %d ... ",
        	minor, port, port+3, irq );
	
    if( 0 > check_region( port, 4 ) ) {
        VERBOSE_MSG( "error(%d) PCLTA_IOPERM.\n", PCLTA_IOPERM );
        return false;
    }

    init_hw( port, 0x00, CLK_10 ); // initialize device hardware, no IRQ

    if( ! wait_reset( port, 500 ) ) {
        VERBOSE_MSG( "error(%d) PCLTA_LRESET.\n", PCLTA_LRESET );
        return false;
    } // waiting to leave reset state

    setup_timeout( 200 );
    while( !pclta_timeout );
    // Why wait? Because, when PCLTA gets out of RESET state, it sets
    // status register BEFORE preparing uplink reset. So, I read the
    // packet of length 0 and get an error. Stupid!

	// wait for reset command response
    if( ! wait_uplink_buffer( port, 500, rx_buffer ) ) {
        VERBOSE_MSG( "error(%d) PCLTA_RESP.  \n", PCLTA_RESP );
        return false;
    }

	// make sure the reset was successful
    if( rx_buffer[0] != 1 || rx_buffer[1] != niRESET ) {
        VERBOSE_MSG( "error(%d) PCLTA_RESP. Reset not successfull. \n", PCLTA_RESP );
        return false;
    }

    // set up internal interrupt register
    write_byte_wait( port, CMD_XFER );
    write_byte_wait( port, 2 );
    write_byte_wait( port, niIRQENA );
    write_byte_wait( port, IRQPLRTY | RESET_ENABLE | DLREADY_ENABLE | DLPBA_ENABLE
	  		| DLBA_ENABLE | ULREADY_ENABLE );

    // write a command to get transciever status
    write_byte_wait( port, CMD_XFER );
    write_byte_wait( port, 1 );
    write_byte_wait( port, niSSTATUS );

	// wait for response
    if( ! wait_uplink_buffer( port, 500, rx_buffer ) ) {
        VERBOSE_MSG( "error(%d) PCLTA_STAT.\n", PCLTA_STAT );
        return false;
    }

	// get transceiver type
    trans = rx_buffer[2]>>3;
    if( ! *pclta_transceiver[trans] ) {
        VERBOSE_MSG( "error(%d) PCLTA_TSCV.\n", PCLTA_TSCV );
        return false; // transciever not supported
    }

	// register character device
    if( 0 > (pclta_major = register_chrdev( 0, "pclta", &pclta_fops )) ) {
        VERBOSE_MSG( "error(%d) PCLTA_MAJOR.\n", PCLTA_MAJOR );
        return false;
    }
    
    // request interrupt permission   OLD LOCATION
//    if( 0 > ( erc = request_irq( irq, pclta_interrupt, 0, "pclta", NULL ) ) ) {   SLOW INT
//    if( 0 > ( erc = request_irq( irq, pclta_interrupt,SA_INTERRUPT, "pclta", device ) ) ) {
//        VERBOSE_MSG( "error(%d) PCLTA_IRQPERM.\n", PCLTA_IRQPERM );
//        return false;
//    }
    
    // create device descriptor
    device = create_device( port, irq, pclta_clock[rx_buffer[2]>>3] );
    if( device == NULL ) {
        VERBOSE_MSG( "error(%d) PCLTA_CDEV.\n", PCLTA_CDEV );
        return false;
    }
    
        // request interrupt permission  NEW LOCATION SO WE HAVE DEVICE STRUCT
//    if( 0 > ( erc = request_irq( irq, pclta_interrupt, 0, "pclta", NULL ) ) ) {   SLOW INT
    if( 0 > ( erc = request_irq( irq, pclta_interrupt_handler,0, "pclta", device) ) ) {
        VERBOSE_MSG( "error(%d) PCLTA_IRQPERM.\n", PCLTA_IRQPERM );
        return false;
    }

    
    // set up pointers to device descriptor
    pclta_device_table[minor] = device;
    pclta_interrupt_table[minor] = device;
    
    // remember minor number
    device->minor = minor;
    
    // claim I/O space
    request_region( port, 4, "pclta" );
    
    // start up the interupts   removed 07/07 by karl.
//    init_hw( port,irq_mask[irq], CLK_10 ); // initialize device hardware, no IRQ

    
    DEBUG_MSG( "Transceiver type: %s", pclta_transceiver[trans] );
    MESSAGE("Device pclta%d installed.", minor );
    return true;
}

//-----------------------------------------------------------------
// init_hw()

void init_hw( int port, byte irq, byte txcvr_clock )
{
	CALL_MSG("init_hw()", ++pclta_call_level );

	DEBUG_MSG("PROGRAM IRQ LINE Writing 0x0%x to 0x0%x",irq, selirq_reg(port));	
	// program interrupt request line
    outb_p( irq, selirq_reg(port) );
    
    // reset device
   	DEBUG_MSG("RESET device with txcvr_clock Writing 0x0%x to 0x0%x",txcvr_clock, clkrst_reg(port));	
    outb_p( txcvr_clock, clkrst_reg(port) );
   	DEBUG_MSG("UNREST Device Writing 0x0%x to 0x0%x", PCLTA_RESET | txcvr_clock, clkrst_reg(port) );
    outb_p( PCLTA_RESET | txcvr_clock, clkrst_reg(port) );


   	DEBUG_MSG("ACK IRQ hardware Writing 0x0%x to 0x0%x",~CLRIRQ, clrirq_reg(port) );
    
    // acknowledge IRQ hardware
    if( irq )
    	outb_p( ~CLRIRQ, clrirq_reg(port) );

	RETURN_MSG("init_hw()", pclta_call_level-- );
}

//-----------------------------------------------------------------
// cleanup_module()

void cleanup_module( void )
{
    unsigned int i;

	CALL_MSG("cleanup_module()", ++pclta_call_level );
	
    for( i=0; i<MAX_DEVICES; i++ )
    	cleanup_hw( pclta_device_table[i] );
	
    unregister_chrdev( pclta_major, pclta_device_name );

	RETURN_MSG("cleanup_module()", pclta_call_level-- );
}

//-----------------------------------------------------------------
// cleanup_hw()

void cleanup_hw( struct pclta_device *device )
{
	int io;
	
	CALL_MSG("cleanup_hw()", ++pclta_call_level );
	
	// don't bother if device undefined
    if( device == NULL ) {
		RETURN_MSG("cleanup_hw() DEV=NULL", pclta_call_level-- );
    	return;
	}

	io = device->base_address;
	
	// reset internal interrupt register
    write_byte_wait( io, CMD_XFER );
    write_byte_wait( io, 2 );
    write_byte_wait( io, niIRQENA );
    write_byte_wait( io, IRQPLRTY );
 
	// release I/O region and interrupt
    release_region( io, 4 );
    
    DEBUG_MSG("Freeing up Interrupt %d", device->interrupt);
    free_irq( device->interrupt,device );
    
    // free up other resources
    remove_device( device );

	RETURN_MSG("cleanup_hw() CLEANED", pclta_call_level-- );
}

// -----------------------------------------------------------------
// registered functions

//---------------------------------------------
// read_pclta()
ssize_t read_pclta( struct file *dev_file, char * buffer, size_t count, loff_t * offset )

//long read_pclta(struct inode* dev_inode, struct file *dev_file, char *buffer, unsigned long count)
//read_write_t read_pclta(struct inode*, struct file *dev_file, char * buffer, count_t count)

{
	struct pclta_device *device = dev_file->private_data;
	struct pclta_packet * packet_buffer;
	byte temp_swap=0;

	CALL_MSG("Starting read_pclta()", ++pclta_call_level );
		
	// verification
	if( device == NULL ) {
		RETURN_MSG("read_pclta() ENXIO", pclta_call_level-- );
		return -ENXIO;
	}
	
//	if (last_packet_read_complete==true)
	
    	// wait for uplink packet
	packet_buffer = device->rx->tail; // read from tail.
    print_buffer(device->rx);
	

	// no data available
	if( ! packet_buffer->busy && (dev_file->f_flags & O_NONBLOCK) ) {
		RETURN_MSG("read_pclta() EWOULDBLOCK", pclta_call_level-- );
		return -EWOULDBLOCK;
	}
	else
		if( ! packet_buffer->busy ) {
    		DEBUG_MSG( "in read_pclta() packet_buffer  ADDY(tail)=%ud", (unsigned int) &packet_buffer);
     		DEBUG_MSG("p_b->length= %d ;data0=; %x data1= %x ", packet_buffer->length,packet_buffer->data[0],packet_buffer->data[1]);

			RETURN_MSG("read_pclta() ENODATA", pclta_call_level-- );
			return -ENODATA;		// nothing available
		}
		
	// check if enough space available to read.
	if( count <= packet_buffer->length ) {
	    DEBUG_MSG("ERROR: count= %d    ;  packet_buffer->length= %d",(int) count, packet_buffer->length);
		RETURN_MSG("read_pclta() EINVAL;  count > packet_buffer->length", pclta_call_level-- );
		return -EINVAL;

	}

    //swap the first two bytes and subtract one from the length
    temp_swap=packet_buffer->data[0]-1;  // subtract one from length
    packet_buffer->data[0]=packet_buffer->data[1];
    packet_buffer->data[1]=temp_swap;
	
	// copy data from buffer to user space
	copy_to_user( buffer, &packet_buffer->data[0], packet_buffer->length+1 );

	// update buffer data
	packet_buffer->busy = false;
	device->rx->tail = packet_buffer->next;
	
	
//	restart uplink if backed off
	if( device->restart_uplink ) {
		disable_irq( device->interrupt );
		device->restart_uplink = false;
		if( device->state == Idle )
			pclta_service( device );
		enable_irq( device->interrupt );
	} // restart uplink, but only if device is in the Idle State

	RETURN_MSG("SUCCESS read_pclta() length = %d", pclta_call_level--, packet_buffer->length );
	
	return packet_buffer->length+1;
}

//---------------------------------------------
// write_pclta()

//int write_pclta( struct file *file, const char *buffer, size_t count, loff_t * offset)						
ssize_t write_pclta( struct file *file, const char *buffer, size_t count, loff_t *offset )

{
    struct pclta_device *device = file->private_data;
    struct pclta_packet *packet_buffer;
    unsigned int length_of_msg;
    byte temp_swap=0;
    byte temp_buffer[512];
    int sum_sent=0;  // sum of bytes sent.
    byte *msg_ptr;    
	
	CALL_MSG("Starting write_pclta()", ++pclta_call_level);

	
	// verification
    if( device == NULL ) {
    	RETURN_MSG("write_pclta() ERROR device=NULL", pclta_call_level-- );
    	return -ENXIO;
	}
	
    if( 0 > verify_area( VERIFY_READ, buffer, count ) ) {
    	RETURN_MSG("write_pclta() ERROR= EFAULT verify area ", pclta_call_level-- );
    	return -EFAULT;
	}
	
    if( count < 2 ) {
    	RETURN_MSG("write_pclta() ERROR=EINVAL  count < 2 ", pclta_call_level-- );
    	return -EINVAL;
	}

	//    length=count; // karl

//    copy_from_user(&packet_buffer->length, buffer, count );  // reads NI command into the legth position

    copy_from_user(temp_buffer, buffer,count );  // reads NI command into the legth position
	msg_ptr=temp_buffer;
	while (sum_sent < count) // must send all data
	{
	
    	// wait for free buffer entry
        packet_buffer = device->tx->head;
    
    
        // no buffer available
        if( packet_buffer->busy && (file->f_flags & O_NONBLOCK) ) {
        	RETURN_MSG("write_pclta()  ERROR write_pclta()", pclta_call_level-- ); 
        	return -EWOULDBLOCK;
    	}
    	else
    		if( packet_buffer->busy ) {
    			RETURN_MSG("write_pclta() ERROR EAGAIN", pclta_call_level-- );
	    		return -EAGAIN;		// retry later
        	}


        length_of_msg=*(msg_ptr+1)+2;
        memcpy(&packet_buffer->length,msg_ptr,length_of_msg);
        

//	DEBUG_MSG("in write_pclta() Length=[%x] D0=[%x] ;D1=[%x] ;D2=[%x] ;D3=[%x] ;D4=[%x] ;D5=[%x] ;D6=[%x]",packet_buffer->length,packet_buffer->data[0],packet_buffer->data[1],packet_buffer->data[2],packet_buffer->data[3],packet_buffer->data[4],packet_buffer->data[5],packet_buffer->data[6]);

      //swap the first two bytes and subtract one from the length
        temp_swap=packet_buffer->data[0];  // [0] is the length
        packet_buffer->data[0]=packet_buffer->length;  // put the NI command at postion 0 
        packet_buffer->length=temp_swap + 1; 
//	DEBUG_MSG("write_pclta() FIX Length=[%x] D0=[%x] ;D1=[%x] ;D2=[%x] ;D3=[%x] ;D4=[%x] ;D5=[%x] ;D6=[%x]",packet_buffer->length,packet_buffer->data[0],packet_buffer->data[1],packet_buffer->data[2],packet_buffer->data[3],packet_buffer->data[4],packet_buffer->data[5],packet_buffer->data[6]);


    	// update output buffers and start downlink transfer
        device->tx->head = packet_buffer->next;
        packet_buffer->busy = true;
    
        sum_sent+=length_of_msg;
	    msg_ptr+=length_of_msg;
    }
    
    if( device->state == Idle ) {
        disable_irq( device->interrupt );
	    DEBUG_MSG("in write_pclta()  about to start_downlink  interupt= %d",device->interrupt);
        pclta_start_downlink( device );
        enable_irq( device->interrupt );
    }

       
    
	RETURN_MSG("write_pclta() finished. ", pclta_call_level-- );
    return count;
}

//---------------------------------------------
// poll_pclta()
//   not used   karl removed   1 aug 2001
/*
unsigned int poll_pclta( struct file * file, struct poll_table_struct * poll )
{
	int mask = 0;
	struct pclta_device * pclta_device_ptr = file->private_data;
	
	CALL_MSG( "poll_pclta()", ++pclta_call_level );
	
	poll_wait(file, &pclta_device_ptr->pclta_wait_queue, poll);
	
	// is the device readable?	
	if( &pclta_device_ptr->rx->head != &pclta_device_pt->rx->tail )
		mask |= POLLIN | POLLRDNORM;
	
	// is the device writeable?	
	if( &pclta_device_ptr->tx->tail->next != &pclta_device_pt->tx->head )
		mask |= POLLOUT | POLLWRNORM;
		
	RETURN_MSG( "poll_pclta() mask = 0x%8x", pclta_call_level--, mask );
    return mask;
}
*/

//---------------------------------------------
// open_pclta()

int open_pclta( struct inode *inode, struct file *file )
{
	struct pclta_device *device = pclta_device_table[MINOR(inode->i_rdev)];
	file->private_data = device;

//	file->f_op=&pclta_fops;  karl test 13/07/2000

	CALL_MSG("open_pclta()", ++pclta_call_level );
	
	if( device == NULL ) {
		RETURN_MSG("open_pclta() ENXIO", pclta_call_level-- );
		return -ENXIO; // no such device
	}
	
	if( device->state != Offline ) {
		RETURN_MSG("open_pclta() EBUSY", pclta_call_level-- );
		return -EBUSY; // device busy
	}
	
	if( !open_device( device ) ) {
		RETURN_MSG("open_pclta() ENOMEM", pclta_call_level-- );
		return -ENOMEM; // out of memory
	}
	
	// program interrupt request line
//	outb_p(irq_mask[device->interrupt], selirq_reg(device->base_address) );  //removed 07/07/200  karl. added init_hw below

    DEBUG_MSG("Base Addy=0x0%x ; IRQ=%d ; IRQ MASK = 0x0%x \n",device->base_address, device->interrupt ,pclta_irq_mask[device->interrupt]);
    init_hw( device->base_address, pclta_irq_mask[device->interrupt], device->txcvr_clock );

    if( ! wait_reset(device->base_address, 500 ) ) {
        VERBOSE_MSG( "error(%d) PCLTA_LRESET.\n", PCLTA_LRESET );
        return -EBUSY;
    } // waiting to leave reset state
	
		
	device->state = Idle;
	
	//enable_irq( device->interrupt );  // karl's test  07/07/2000

	MOD_INC_USE_COUNT;
	
	RETURN_MSG("open_pclta() OK", pclta_call_level-- );
	return 0;
}

//---------------------------------------------
// close_pclta()

int close_pclta( struct inode *inode, struct file *file )
{
    struct pclta_device *device = pclta_device_table[MINOR(inode->i_rdev)];

	CALL_MSG("close_pclta()", ++pclta_call_level );
	
    if( device == NULL ) {
		RETURN_MSG("close_pclta() ENXIO", pclta_call_level-- );
    	return -ENXIO; // no such device
	}
    if( device->state == Offline ) {
		RETURN_MSG("close_pclta() already closed", pclta_call_level-- );
    	return 0; // device already closed
	}
// karl removed this for 2.4 kernel 30 July 2001
//    if( file->f_count >1 ) {
//		RETURN_MSG("close_pclta() EBUSY", pclta_call_level-- );
//    	return -EBUSY; // not the last process
//    }
    
    device->state = Offline;
    init_hw( device->base_address, 0x00, device->txcvr_clock );  // turn off interupts
    close_device( device );
    MOD_DEC_USE_COUNT;

	RETURN_MSG("close_pclta()", pclta_call_level-- );
	
    return 0;
}

//-----------------------------------------------------------------
// timer dependent read/write operations

// write_byte_timeout()

bool write_byte_timeout( int port, byte data_byte, int sec100 )
{
	CALL_MSG("write_byte_timeout()", ++pclta_call_level );

    setup_timeout( sec100 );
    while( ( inb_p( status_reg(port) ) & _READY ) && ( ! pclta_timeout ) );
    
    if( ! pclta_timeout )
    	outb_p( data_byte, data_reg(port) );
	
    del_timer( &pclta_timer_list );
    
    if( pclta_timeout ) {
    	RETURN_MSG( "write_byte_timeout() false", pclta_call_level-- );
		return false;
	}
	
   	RETURN_MSG( "write_byte_timeout() true", pclta_call_level-- );
    return true;
}

// wait_reset()

bool wait_reset( unsigned int port, int sec100 )
{
	CALL_MSG("wait_reset()", ++pclta_call_level );
	
    setup_timeout( sec100 );
    
    while( ( inb_p(status_reg(port)) & RESET ) && !pclta_timeout );
    
    del_timer( &pclta_timer_list );
    
    if( pclta_timeout ) {
		RETURN_MSG("wait_reset() false", pclta_call_level-- );
    	return false;
	}
	
	RETURN_MSG("wait_reset() true", pclta_call_level-- );
    return true;
}

// wait_uplink_buffer()

bool wait_uplink_buffer( int port, int sec100, byte *buffer )
{
	int count=0;

	CALL_MSG("wait_uplink_buffer()", ++pclta_call_level );
	
	
	// initialize timeout timer
    setup_timeout( sec100 );
    
    // wait for uplink buffer, or timeout
    while( !(inb_p(status_reg(port)) & ULREADY) && !pclta_timeout )
    {
    	count++;
	}
    
    // uplink buffer ready, kill timeout timer
    del_timer( &pclta_timer_list );
    
    // unsuccessful if timed out
    if( pclta_timeout ) {
		RETURN_MSG("wait_uplink_buffer() TIMEOUT", pclta_call_level-- );
    	return false;
	}
	
	// otherwise, ready to send packet to device
    uplink_packet( port, buffer );
    DEBUG_MSG("Count = %d",count);
	RETURN_MSG("wait_uplink_buffer()", pclta_call_level-- );
    return true;
}

// setup_timeout()

void setup_timeout( int sec100 )
{
	//CALL_MSG("setup_timeout()", ++pclta_call_level );
	
    init_timer( &pclta_timer_list );
    pclta_timer_list.data = 0;
    pclta_timer_list.expires = jiffies + sec100 * (HZ/100);
    pclta_timer_list.function=pclta_timer_handler;
    add_timer( &pclta_timer_list );
    pclta_timeout = 0;

	//RETURN_MSG("setup_timeout()", pclta_call_level-- );
}

//-----------------------------------------------------------------
// interrupt handlers


// pclta_start_downlink()


// if( state==Idle AND downlink_packet_pending )
// |   if( downlink_buffer_avaiable AND interface_not_reset )
// |   |   start_downlink_transfer;
// |   |   state = Transfer;
// |   endif
// endif

void pclta_start_downlink( struct pclta_device *dev )
{
    struct pclta_packet *packet_buffer;
    unsigned char ni_cmd, buff_p, status;

	CALL_MSG("Starting pclta_start_downlink()", ++pclta_call_level );
//	DEBUG_MSG
	
    if( dev->state == Idle ) {
        DEBUG_MSG("pclta_start_downlink () Device is idle.  checking if busy");

        packet_buffer = dev->tx->tail;
        if( packet_buffer->busy ) {
            ni_cmd = packet_buffer->data[0];
	        DEBUG_MSG("NICOMMAND = [0x%2x]",ni_cmd);
            if( ni_cmd == niSERVICE ) {
                buff_p = DLPBA;
                ni_cmd = niCOMM;
            }
	    	else {
                buff_p = ni_cmd & niPRIORITY ? DLPBA : DLBA;
                ni_cmd &= NI_Q_CMD;
            }
	    
            status = inb_p( status_reg(dev->base_address) );
	    
            if( ni_cmd == niCOMM || ni_cmd == niNETMGMT ) {
                if( status & buff_p )
                    dev->state = Transfer;
            }
	    	else
                dev->state = Transfer;
		
            if( dev->state == Transfer ) {
                if( status & RESET )
                    dev->state = Idle;
                else {
                    DEBUG_MSG( "xfer_start, " );
                    write_byte_wait( dev->base_address, CMD_XFER );
                    DEBUG_MSG( "xfer_done, base addy = 0x0%x ",dev->base_address );

                }
            }
        }
    }
	RETURN_MSG("Finishing pclta_start_downlink()", pclta_call_level-- );
}

// pclta_service()

// read_device_state;
// if( device_resetting )
// |   state = Idle
// |   return;
// endif
// if( state==Transfer )
// |   complete_downlink_transfer;
// |   state = Idle;
// |   read_device_state;
// if( uplink_packet_pending )    --- notice: state==Idle
// |   if( uplink_buffer_available )
// |   |   uplink_packet;
// |   else
// |   |   restart_uplink=true;
// |   endif
// endif
// start_downlink;

void pclta_service( struct pclta_device *device )
{
    int io = device->base_address;
    struct pclta_packet * packet_buffer;
    byte status;

	DEBUG_MSG("pclta_service()");
	
    status = inb_p( status_reg(io) );
    DEBUG_MSG( "status: %#x, ", status );
    
    if( status & RESET ) {
        DEBUG_MSG( "in PCLTA_SERVICE() Device reset.  State=idle.  returning.. " );
        device->state = Idle;
        return;
    }
    
    if( device->state == Transfer ) {
        DEBUG_MSG( "xfer_body, " );
        packet_buffer = device->tx->tail;
//        downlink_packet( io, & packet_buffer->data[0] );   old    karl removed 17/07/00
        downlink_packet( io, &packet_buffer->length );   
        packet_buffer->busy = false;
        device->tx->tail = packet_buffer->next;
        device->state = Idle;
        status = inb_p( status_reg(io) );
        DEBUG_MSG( "status: %#x, ", status );
    }
    
    if( status & ULREADY ) {
        DEBUG_MSG( "in PCLTA_SERVICE()  ULREADY. " );

        packet_buffer = device->rx->head;
        if( !packet_buffer->busy ) {
            DEBUG_MSG( "in PCLTA_SERVICE()  packet_buffer is not busy. (open spot in bufer) " );

            if( uplink_packet( io, &packet_buffer->data[0] ) ) {
	            DEBUG_MSG( "in PCLTA_SERVICE() Seting packet_buffer->busy=true  ADDY=%ud",(unsigned int) &packet_buffer);
		        DEBUG_MSG( "in PCLTA_SERVICE() pb->length= %d",packet_buffer->length);
		        packet_buffer->busy= true;
			    packet_buffer->length=packet_buffer->data[0];  // Karll added  13/07/2000
                device->rx->head = packet_buffer->next;

				print_buffer(device->rx);
		
            } else  DEBUG_MSG( "in PCLTA_SERVICE. UPLINK_PACKET() returned FALSE");
	    
	        
        }
		else
            device->restart_uplink = true;
    }
    
    pclta_start_downlink( device );
}


// pclta_interrupt()

void pclta_interrupt_handler( int irq, void * device_id, struct pt_regs * regs )
{
    struct pclta_device * device = device_id ;

	DEBUG_MSG("pclta_interrupt()");
	
    if( device == NULL )
    	return; // no such device ???
	
    if( device->state == Offline ) {
    	DEBUG_MSG( "device offline" );
    	return; // ignore the interrupt
	}
	
	// acknowledge interrupt
    outb_p( ~CLRIRQ, clrirq_reg(device->base_address) );
    DEBUG_MSG( "IRQ#%d received, minor = %d", irq, device->minor );
    
    // service the interrupt
    device->interrupt_queue->routine = (void *)pclta_service;
    device->interrupt_queue->data = (void *)device;
    device->interrupt_queue->sync = 0;
    queue_task( device->interrupt_queue, &tq_immediate );
    mark_bh(IMMEDIATE_BH);
    
    //pclta_service( device ); 
    //wake_up_interruptible( & device->pclta_wait_queue );
    DEBUG_MSG( "IRQ#%d serviced, minor = %d", irq, device->minor );
	
	return;
}

//-----------------------------------------------------------------
// timer_handler()

void pclta_timer_handler( unsigned long data )
{
    pclta_timeout = -1;
    return;
}

//-----------------------------------------------------------------
// packet read/write functions

// uplink_packet()

bool uplink_packet( int port, byte *buffer )
{
    byte length;

	CALL_MSG("starting uplink_packet()", ++pclta_call_level);
	
    write_byte_wait( port, CMD_ACK );
    read_byte_wait( port ); // dummy
    *buffer++ = length = (read_byte_wait( port ));  //first byte is length 
    
    DEBUG_MSG( "in uplink_packet() length = %d", length );
    
    
    if( length == 0 ) {
    	RETURN_MSG("uplink_packet() LENGTH=0", pclta_call_level-- );
    	return false; // drop it
	}
	
    while( length-- ) {
        *buffer++ = read_byte_wait( port );
		#ifdef DEBUG
			if((int)(buffer-1)%8 == 0)  // formating debug output.. 
				DEBUG_MSG("                    " );
        	printk( "[%2x] ", *(buffer-1) );
		#endif	
		CALL_MSG("downlink_packet()", ++pclta_call_level );

    }
    
    RETURN_MSG("finishing uplink_packet()", pclta_call_level-- );
    return true;
}

// downlink_packet()

void downlink_packet( int port, byte * buffer )
{
    byte length = *buffer;   //length is first byte in buffer.

	CALL_MSG("downlink_packet()", ++pclta_call_level );
	
	DEBUG_MSG("IN downlink_packet() LENGTH= %d",length);
	
    write_byte_wait( port, length );
    
    while( length-- )
    	write_byte_wait( port, *(++buffer) );
	
	RETURN_MSG("downlink_packet()", pclta_call_level-- );

}


//-----------------------------------------------------------------
// inline functions

// read_byte_wait()

inline byte read_byte_wait( int port )
{
    byte read_byte;


    while( inb_p( status_reg(port) ) & _READY );
    read_byte= inb_p( data_reg(port) );
    
    if ((read_byte > 32) && (read_byte < 127))
 	DEBUG_MSG("in read_byte_wait() Returning [0x%x] '%c' ",read_byte,read_byte);
	else 	DEBUG_MSG("in read_byte_wait() Returning [0x%x]",read_byte);

    return(read_byte);
    
}

// write_byte_wait()

inline void write_byte_wait( int port, byte data_byte )
{
    int count=0;
    
    while( inb_p( status_reg(port) ) & _READY )
	    count++;

   if ((data_byte > 32) && (data_byte < 127))
    DEBUG_MSG("write_byte_wait() count = %d ; byte=0x0%x '%c' port=0x0%x",count, data_byte,data_byte, data_reg(port));
   else DEBUG_MSG("write_byte_wait() count = %d ; byte=0x0%x  port=0x0%x",count, data_byte, data_reg(port));
    outb_p( data_byte, data_reg(port) );
}


//-----------------------------------------------------------------
// device data structure maintenance functions

// create_device()
//
// initialize device descriptor (hardware parameters)

struct pclta_device * create_device( int port, int irq, byte txcvr_clock )
{
    struct pclta_device *device;

	CALL_MSG( "create_device()", ++pclta_call_level );
	
	// allocate descriptor space
    device = kmalloc( sizeof(struct pclta_device), GFP_KERNEL );
    if( device == NULL ) {
		RETURN_MSG( "create_device() kmalloc error", pclta_call_level-- );
    	return NULL;
	}
	else {
		device->interrupt_queue = kmalloc( sizeof( struct tq_struct ), GFP_KERNEL );
		if( device->interrupt_queue == NULL ) {
			kfree( device );
			RETURN_MSG( "create_device() kmalloc error", pclta_call_level-- );
			return NULL;
		}
	}
	
    device->base_address = port;
    device->interrupt = irq;
    device->state = Offline;
    device->rx = device->tx = NULL; // initialize these later
    device->txcvr_clock = txcvr_clock;

	// initialize interrupt task queue
    device->interrupt_queue->routine = (void *)pclta_service;
    device->interrupt_queue->sync = 0;
    
	RETURN_MSG( "create_device()", pclta_call_level-- );
    return device;
}

// remove_device()

void remove_device( struct pclta_device *device )
{
    if( device == NULL ) // device doesn't exist
    	return;
	
    close_device( device );
    kfree( device->interrupt_queue );
    kfree( device );
}

// open_device()
//
// open memory resources, initialize device state
// initialize device descriptor (software parameters)

bool open_device( struct pclta_device * device )
{
	CALL_MSG( "Starting open_device()", ++pclta_call_level );
	
	// allocate and initialize buffers
	device->rx = kmalloc( sizeof(struct pclta_packet_buffer), GFP_KERNEL );
	if( device->rx == NULL ) {
		RETURN_MSG( "open_device() ERROR= device->rx == NULL", pclta_call_level-- );
		return false;
	}
	device->tx = kmalloc( sizeof(struct pclta_packet_buffer), GFP_KERNEL );
	if( device->tx == NULL ) {
		kfree( device->rx );
		RETURN_MSG( "open_device() ERROR= device->tx == NULL", pclta_call_level-- );
		return false;
	}
	
	init_buffers( device->rx );
	DEBUG_MSG("RECEIVE BUFFER");
	print_buffer(device->rx);

	init_buffers( device->tx );
	DEBUG_MSG("TRANSMIT BUFFER");
	print_buffer(device->tx);

		
	// set initial state of device
	device->restart_uplink = false;
	device->pclta_wait_queue = NULL;
//    last_packet_read_complete=true;	 // for incomplete reads
    
	RETURN_MSG( "open_device() SUCCESS", pclta_call_level-- );
	return true;
}

// close_device()
// 
// take device off line and release buffer memory

void close_device( struct pclta_device * device )
{
	CALL_MSG( "close_device()", ++pclta_call_level );
	
    if( device->state == Offline ) {
    	RETURN_MSG( "close_device()", pclta_call_level-- );
    	return;
	}
	
    kfree( device->rx );
    kfree( device->tx );
    device->tx = device->rx = NULL;
    
//    wake_up_interruptible( &device->pclta_wait_queue ); //  karl thinks this not needed  removed 1 aug 2001

    RETURN_MSG( "close_device()", pclta_call_level-- );
}

// init_buffers()

void init_buffers( struct pclta_packet_buffer * packet_buffer )
{
	unsigned int i;
	int buffer_index;
	
	CALL_MSG( "init_buffers()", ++pclta_call_level );
	
	for( i=0; i<PACKETS_PER_BUFFER; i++ ) {
		packet_buffer->buffer[i].next = & packet_buffer->buffer[(i+1)%PACKETS_PER_BUFFER];
		packet_buffer->buffer[i].busy = false;
		packet_buffer->buffer[i].length = 0;
		for (buffer_index=0; buffer_index < MAX_PACKET_SIZE; buffer_index++)  // default data to 0
		    packet_buffer->buffer[i].data[buffer_index] = 0;

	 
	
	}
	// start at beginning of buffer

	packet_buffer->head = packet_buffer->tail = &packet_buffer->buffer[0];

	RETURN_MSG( "init_buffers()", pclta_call_level-- );
}

