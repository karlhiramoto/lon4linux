/*
 *  lpclinux.c
 *
 *  Driver for "state of the art" LonWorks interfaces
 *  Version 1.02b, Apr 2004
 *  based on the generic LPC/LPP driver written by Ralf Feiland
 *
 *  History:
 *  Nov 1998 - Implementation of the generic driver
 *  Mar 1999 - Windows CE driver based on the generic driver released
 *  Mar 2000 - First beta release (0.90) of the linux driver
 *  Jun 2000 - Beta release 0.91 bugfixed with IRQ sharing and module parameters
 *  Oct 2000 - Beta release 0.92 with protocol analyzer interface (Watcher)
 *  Aug 2002 - Release 1.00 running also on Linux 2.4
 *  Jan 2003 - Release 1.01 bugfixed in driver unload,
 *             optional ISA HW check at load time (CHECK_ISA_AT_LOAD_TIME)
 *  Apr 2004 - Release 1.02 some debug output for 
 *             kernel panic in wake_up_interruptible(&dev->rd_wait);
 *  Jun 2004 - Release 2.02 running on Linux 2.6 and 2.4
 *             using PCI-PnP
 *  Jun 2004 - do not use wake_up_interruptible_sync(), only wake_up_interruptible(),
 *             it's very slow on Kernel 2.6.5
 *  Nov 2005 - Release 2.08 used copy_to_user(), copy_from_user(), __get_user(), 
 *             __put_user() instead direct access to user memory
 *  Dec 2005 - Release 2.09 evaluate return value from copy_to_user() and copy_from_user()
 *
 * This product may be distributed under the terms of the GNU Public License.
 *
 *
 * Compiling instructions:
 *
 *  Only if you want to build a static driver,
 *  insert a unique major number in /usr/include/linux/major.h:
 *  Example: #define LPC_MAJOR    84
 *
 *  Then compile the driver for Linux 2.2:
 *  gcc -c -Wall -D__KERNEL__ -DMODULE -O2 lpclinux.c
 *
 *  or for Linux 2.4:
 *  gcc -c -Wall -D__KERNEL__ -DMODULE -O2 -I/usr/src/linux/include lpclinux.c
 *
 *  Add some nodes in the /dev directory:
 *  Minor number  0..31 are used for ISA cards
 *  Minor number 32..63 are used for PCI cards
 *
 *  Example:
 *  mknod /dev/lon1 c 84 0        (lon1 is the 1st ISA card)
 *  mknod /dev/lon2 c 84 1        (lon2 is the 2nd ISA card)
 *  mknod /dev/lon3 c 84 32       (lon3 is the 1st PCI card)
 *  mknod /dev/lon4 c 84 33       (lon4 is the 2nd PCI card)
 *
 *  Don't forget to set the access rights:
 *  chmod 666 /dev/lon*
 *
 *  Load the driver:
 *  insmod lpclinux.o [params]
 *
 *  If you build a driver module (-DMODULE) instead of a static driver,
 *  the major number is set at load time, so it is not necessary to
 *  change the file major.h.
 *  Example:
 *  insmod lpclinux.o major=84
 *
 *  The I/O addresses and IRQs of ISA cards can now be set at load time, so
 *  it is not necessary to change the driver code.
 *  ISA parameters are defined here as 16 bit values (4 hex digits)
 *    Bits  11..0:  base I/O address
 *    Bits 15..12:  IRQ, valid values are 3,5,7,9,A,B,C and F.
 *
 *  The parameter isa is a string with 4 hex digits for each card.
 *  All characters except 0..9, A..F and a..f are ignored and may be used
 *  as seperators.
 *  Also some other characters like comma and double quote are not allowed
 *  by the insmod command.
 *
 *  Example:
 *  1st card has address 0x340 and IRQ 5
 *  2nd card has address 0x344 and IRQ 7
 *
 *  insmod lpclinux.o isa="53407344"
 *  or
 *  insmod lpclinux.o isa="5/340 7/344"
 *  or
 *  insmod lpclinux.o isa="irq=5 port=340h : irq=7 port=344h"
 *
 *  Not allowed is
 *  insmod lpclinux.o isa="interrupt = 5, baseaddress = 0x340"
 *                            ^         ^ ^^ ^^   ^     ^
 */

#include "lpclinux.h"

//#include "kdebug.c"
#include "driverdata.c"
//#include "dynbuf.c"




static int downlink( DEV *dev )
{
	u8 cmq, len, st;
	u8 *p;
	int idx;
	u32 cport, wport;

    dbg8("%s(): dev=%08X", __FUNCTION__, (uint)dev);

    cport = dev->cport;
	wport = dev->wport;
	idx = dev->outhead;
	p = dev->outbuf[idx];
#if 0
	len = *p++;
    cmq = *p++;
#else
    cmq = *p++;
	len = *p++;
//    len ++;
#endif
    if (debug & DEBUG_DUMP2)  DumpBuffer(">>", p-2, len+2);
//    printk(KERN_DEBUG ">len=%02X cmq=%02X\n", len, cmq);
	HS;
	NOUT(CMD_XFER);
	HS;
	NOUT(len+1);
	HS;
	NOUT(cmq);
	HS;
//    printk(KERN_DEBUG "len=%02X Loop: ", len);
    while(len--) 
        {
//        printk ("%.2x ", *p);
		HS;
		NOUT(*p++);
        }
//    printk ("\n");
	HS;
	NOUT(EOM);
	idx++;
	if(idx >= NUM_OUTBUF) idx=0;
	dev->outhead = idx;
	return 0;
}


static int uplink( DEV *dev )
    {
    u8 cmq, len, st;
    u8 *p;
    int idx;
    u32 cport, rport;
    
    dbg8("%s(): dev=%08X", __FUNCTION__, (uint)dev);
    
    cport = dev->cport;
    rport = dev->rport;
    HS;
    if( !NIN ) return 0;          // Pass token
    HS;
    len = NIN;
    HS;
    cmq = NIN;
    if(cmq == 0xC0)  return 1;    // niACK received
    
    idx = dev->intail;            // uplink
    p = dev->inbuf[idx];
#if 0
    *p++ = len;
    *p++ = cmq;
    len--;
#else
    *p++ = cmq;
    len--;
    *p++ = len;
#endif
//    printk(KERN_DEBUG "<len=%02X cmq=%02X\n", len, cmq);
    while( len-- ) 
        {
        HS;
        *p++ = NIN;
        }
    if (debug & DEBUG_DUMP2)
        {
        p    = dev->inbuf[idx];
        len  = p[1];
        DumpBuffer("<<", p, len+2);
        }
    idx++;
    if(idx >= NUM_INBUF) idx=0;
    if(idx != dev->inhead) 
        {
        dev->intail = idx;
        dbg8("  %s(): before waitqueue_active()", __FUNCTION__);
        if (waitqueue_active(&dev->rd_wait))
            {
            dbg8("  %s(): before wake_up_interruptible()", __FUNCTION__);
            wake_up_interruptible( &dev->rd_wait );
            }
        else
            dbg8("  %s(): waitqueue_active()=0", __FUNCTION__);
        }
    return 0;
    }

static int sm_lpc(register DEV *dev)
{
	u8  *p;
	u8  st;
	int g;
//	int flags;
	u16 idx;                  // avoid critical section
	u32 cport,  rport, wport;

    dbg4("  %s(): dev=%08X", __FUNCTION__, (uint)dev);
    
	cport = dev->cport;
	rport = dev->rport;
	wport = dev->wport;
	switch( dev->state )
	{
		case ST_RESYNC:
			if(LPCSTAT & 1) return 0;
			dev->ctl &= ~4;
			SETCTL;                             // clear reset flipflop
			SETCTL;
			SETCTL;
			dev->ctl |= 4;
			SETCTL;
			HS;
			NOUT(CMD_RESYNC);                   // send RESYNC
			HS;
			NOUT(EOM);
			HS;
			if(NIN != CMD_ACKSYNC) return -1;   // get ACKSYNC
			dev->state = ST_RESET;
			return 0;

		case ST_RESET:
			HS;
			NOUT(CMD_NULL);                    // send NULL
			HS;
			NOUT(EOM);
			dev->state = ST_FLCAN;
			return 0;

		case ST_FLCAN:
			HS;
			if(NIN != CMD_XFER) return -1;	 // get niRESET
			HS;
			if(NIN != 1) return -1;
			HS;
			if(NIN != 0x50) return -1;

//			dev->intail  = dev->inhead;
//			dev->outhead = dev->outtail;
			idx = dev->intail;
			p = dev->inbuf[idx];
			*p++ = 0x50;
			*p++ = 0x00;
			idx++;
			if(idx >= NUM_INBUF) idx=0;
            if(idx != dev->inhead) 
                {
                dev->intail = idx;
                dbg8("  %s(): before waitqueue_active()", __FUNCTION__);
                if (waitqueue_active(&dev->rd_wait))
                    {
                    dbg8("  %s(): before wake_up_interruptible()", __FUNCTION__);
                    wake_up_interruptible( &dev->rd_wait );
                    }
                else
                    dbg8("  %s(): waitqueue_active()=0", __FUNCTION__);
                }
			HS;
			NOUT(CMD_XFER);			 // send FLUSH_CANCEL
			HS;
			NOUT(1);
			HS;
			NOUT(0x60);
			HS;
			NOUT(EOM);
			dev->state = ST_UPLINK;
			return 0;

		case ST_IDLE:
			if(dev->outhead == dev->outtail) {
				HS;
				NOUT(CMD_NULL);			// send NULL
				HS;
				NOUT(EOM);
				dev->state = ST_UPLINK;
				return 0;
			}
			p = dev->outbuf[dev->outhead];
			if( !((p[0]-0x10) & 0xE0) ) 
                {     // 1x or 2x
                HS;
                NOUT(CMD_XFER);               // buffer request
                HS;
                NOUT(0x01);
                HS;
                NOUT(p[0]);
                HS;
                NOUT(EOM);
                HS;
                dev->state = ST_NIACK;
                return 0;
                }
			if( downlink( dev ) ) return -1;
			dev->state = ST_UPLINK;
			return 0;

		case ST_UPLINK:
			if( uplink( dev ) < 0 ) return -1;
			dev->state = ST_IDLE;
			if(dev->outhead != dev->outtail) {
//				CLI;
				dev->active++;
//				STI;
			}
			return 0;

		case ST_NIACK:
			g = uplink( dev );
			if( g < 0 ) return -1;
			if( g == 1 ) {
				downlink( dev );
				dev->state = ST_UPLINK;
				return 0;
			}
			HS;
			NOUT(CMD_NULL);			// send NULL
			HS;
			NOUT(EOM);
			dev->state = ST_NIACK;
			return 0;
	}
	return 0;
}



#ifndef NO_PCI
static void DpcForIsr(unsigned long dev_addr)
    {
    DEV *dev = (DEV *) dev_addr;
    dbg4(" %s(): dev=%08X active=%X", __FUNCTION__, (uint)dev, dev->active);

//    do  
        {
        int result;
        u32 cport;
        result = sm_lpc( dev );
        if (result < 0)
            {
            dev->active = 1;  // will be 0
            dev->state = 0;
            cport = dev->cport;
            dbg4(" %s(): active=%X  RST_CMND:", __FUNCTION__, dev->active);
            dev->ctl |= 0x01;   // RST_CMND
            SETCTL;
            SETCTL;
            dev->ctl &= 0xFE;
            SETCTL;
            }
        --dev->active;
        }
//        while (--dev->active > 0);

    dev->drvtimer.expires = jiffies + HZ/5;
    add_timer( &dev->drvtimer );

    dbg4(" %s(): dev=%08X active=%X  return", __FUNCTION__, (uint)dev, dev->active);
    }
#endif


#if 0
// tasklet does not work with ISA-Card !!!
static void sm_stub( DEV *dev )
    {
    dbg4(" %s(): dev=%08X active=%X", __FUNCTION__, (uint)dev, dev->active);
    ++dev->active;
	tasklet_schedule(&dev->Dpc);
    }
#else
static BOOLEAN sm_stub( DEV *dev )
    {
    int result;
//    int flags;
    u32 cport;
    
    dbg4(" %s(): dev=%08X active=%X", __FUNCTION__, (uint)dev, dev->active);
    
    if (++dev->active > 1)  
        {
        dbg4(" %s(): active=%X", __FUNCTION__, dev->active);
        return FALSE;
        }
    do  {
        result = sm_lpc( dev );
        if (result < 0)
            {
            dev->active = 1;  // will be 0
            dev->state = 0;
            cport = dev->cport;
            dev->ctl |= 0x01;
            SETCTL;
            SETCTL;
            dev->ctl &= 0xFE;
            SETCTL;
            }
        } while (--dev->active > 0);

    dev->drvtimer.expires = jiffies + HZ/5;
    add_timer( &dev->drvtimer );
    return TRUE;
    } 
#endif

static irqreturn_t lpc_isa_interrupt( int irq, void *ptr, struct pt_regs *regs )
    {
    DEV *dev;
    
    dev = (DEV *)ptr;
    if( inb(dev->cport) & 8 ) return IRQ_NONE;		// Its not for me
    
    dbg2("+ %s(): dev=%08X", __FUNCTION__, (uint)dev);
    
    del_timer( &dev->drvtimer );
    outb(0, dev->rport);			// Clear interrupt flipflop (ISA)
    sm_stub( dev );
    
    dbg2("- %s(): dev=%08X return", __FUNCTION__, (uint)dev);
	return IRQ_HANDLED;
    }

#ifndef NO_PCI
static irqreturn_t lpc_pci_interrupt( int irq, void *ptr, struct pt_regs *regs )
    {
    DEV *dev = (DEV *)ptr;
    u32 cport;
    
    u16 IntCsr = inw(dev->PlxIntCsrPort);
//    dev->PciIntCount++;
//    info("? %s(): dev=%08X IntCsr=%04X", __FUNCTION__, (uint)dev, IntCsr);
    if (IntCsr == 0xFFFF)  return IRQ_NONE;		// Can't read PLX
    if (inb(dev->cport) & 8)  return IRQ_NONE;		// Its not for me
    
    dbg2("+ %s(): dev=%08X", __FUNCTION__, (uint)dev);
    
    del_timer( &dev->drvtimer );

    cport = dev->cport;
    dev->ctl &= 0xEF;				// Clear interrupt flipflop (PCI)
    SETCTL;
    SETCTL;
    SETCTL;
    dev->ctl |= 0x10;               // CLR_IFF_CMND
    SETCTL;

    sm_stub(dev);
    dbg2("- %s(): dev=%08X return", __FUNCTION__, (uint)dev);
    return IRQ_HANDLED;
    }
#endif

static void lpc_timer( unsigned long ptr )
    {
    DEV *dev = (DEV *)ptr;

    dbg1("+ %s(): dev=%08X", __FUNCTION__, (uint)dev);
    sm_stub(dev);
    
    dbg1("- %s(): dev=%08X return", __FUNCTION__, (uint)dev);
    }

static ssize_t lpc_write( struct file *file, const char *buf,
						  size_t count, loff_t *off )
    {
	u8 *p;
	int idx;
	int len;
//	int flags;
	register DEV *dev = (DEV *)(file->private_data);

    dbg1("%s(): dev=%08X", __FUNCTION__, (uint)dev);
//    DbgFlush();

	if(count < 0) return -EINVAL;
	if(count > 256) return -EINVAL;

	idx = dev->outtail + 1;
	if(idx >= NUM_OUTBUF) idx = 0;
//	CLI;
	if(idx == dev->outhead) 
        {
//		STI;
        dbg1("%s(): dev=%08X  return -EWOULDBLOCK", __FUNCTION__, (uint)dev);
		return -EWOULDBLOCK;
        }
//	STI;
	p = dev->outbuf[ dev->outtail ];
	if (copy_from_user(p, buf, count))
        return -EFAULT;

    if (debug & DEBUG_DUMP1)  DumpBuffer("> ", p, count);
#if 0
// app layer -> link layer
	len = p[1] + 1;
	p[1] = p[0];
	p[0] = (u8)len;
    len++;
#else
	len = p[1] + 2;
#endif
	dev->outtail = idx;

	if( dev->state == ST_IDLE ) 
        {
		del_timer( &dev->drvtimer );
		sm_stub( dev );
        }
    dbg1("%s(): dev=%08X  return %d", __FUNCTION__, (uint)dev, len);
	return len;
    }


static ssize_t lpc_read( struct file *file, char *buf,
						 size_t count, loff_t *off )
    {
//	int flags;
	int idx;
	int len;
	u8 *p;
	register DEV *dev = (DEV *)(file->private_data);
	if(count < 0) return -EINVAL;

    dbg1("%s(): dev=%08X", __FUNCTION__, (uint)dev);
//    DbgFlush();
    
	idx = dev->inhead;
#if 0
	CLI;
	if( idx == dev->intail ) 
        {
		if(file->f_flags & O_NONBLOCK) 
            {
			STI;
			return -EWOULDBLOCK;
            }
		interruptible_sleep_on( &dev->rd_wait );
/*
    The next line produces compiling errors, so I use a workaround
    if( current->signal & ~current->blocked ) return -ERESTARTSYS;
 */
		if( idx == dev->intail ) return -ERESTARTSYS;
        }
	STI;
#else
//    if (qempty(&dev->ReadBufferQueue))
	if (idx == dev->intail)
        {
        dbg1("%s(): qempty", __FUNCTION__);
//        up (&dev->sem);
		if (file->f_flags & O_NONBLOCK)
            {
            dbg1("%s()=-EAGAIN !!!", __FUNCTION__);
            return -EAGAIN;  // -EWOULDBLOCK;
            }

//        if (wait_event_interruptible(dev->rd_wait, qhasdata(&dev->ReadBufferQueue)))
        if (wait_event_interruptible(dev->rd_wait, (idx != dev->intail) ))
            {
            dbg1("%s(): dev=%08X  return -ERESTARTSYS", __FUNCTION__, (uint)dev);
            return -ERESTARTSYS; /* signal: tell the fs layer to handle it */
            }
        dbg8("%s(): nach wait_event_interruptible(rd_wait)", __FUNCTION__);
//        if (down_interruptible(&dev->sem))
//            return -ERESTARTSYS;
        }
        
#endif
	p = dev->inbuf[idx];
#if 0
// link layer -> app layer
	len = p[0];
	p[0] = p[1];
	p[1] = (u8)(len - 1);
	if( count > len + 1 ) count = len + 1;
#else
	len = p[1]+2;
	if (count > len)  count = len;
#endif

    if (debug & DEBUG_DUMP1)  DumpBuffer("< ", p, count);
    if (copy_to_user(buf, p, count))
        return -EFAULT;

	idx++;
	if(idx >= NUM_INBUF) idx = 0;
	dev->inhead = idx;

    dbg1("%s(): dev=%08X  return %d", __FUNCTION__, (uint)dev, count);
	return count;
    }


static unsigned lpc_poll( struct file *file, struct poll_table_struct *wait)
    {
    DEV *dev = file->private_data;
    unsigned mask = 0;
    
    dbg1("%s(): dev=%08X", __FUNCTION__, (uint)dev);
//    DbgFlush();
    
    poll_wait( file, &dev->rd_wait, wait);
    if(dev->inhead != dev->intail) mask |= POLLIN | POLLRDNORM;
    return mask;
    }


static int initialize_isacard( DEV *dev, int index )
{
	u32 cport, rport;
	int irq;

    dbg1("%s(): dev=%08X", __FUNCTION__, (uint)dev);
/*
	if( !lpcs[index] ) {
		err( "invalid minor number %d\n", index );
		return -ENODEV;
	}
	iobase = lpcs[index] & 0x0FFF;
	irq    = lpcs[index] >> 12;
	cport = iobase+1;
	rport = iobase+2;
*/
	cport = dev->cport;
	rport = dev->rport;
	irq   = dev->irq;

#ifndef CHECK_ISA_AT_LOAD_TIME
        {
        int hwfail = 0;
        if( ((u8)irq > 15) || !irq_table[irq] ) 
            {
            err("IRQ%d not supported\n", irq );
            return -ENODEV;
            }
        if( check_region(iobase, 4) ) 
            {
            err( "unable to register port 0x%X\n", iobase);
            return -ENODEV;
            }
        
        outb(0x5F, cport);
        outb(0, rport);
        outb(0, rport);
        outb(0, rport);
        if( (inb(cport) & 0xF0) != 0x50 )  hwfail = 1;
        outb(0x0F, cport);
        outb(0, rport);
        outb(0, rport);
        outb(0, rport);
        if( inb(cport) & 0xF0 )  hwfail = 1;
        if( hwfail ) 
            {
            err("interface card not responding at port 0x%X\n", iobase);
            return -ENODEV;
            }
        request_region( iobase, 4, DRIVER_DEV_NAME );
        }
#endif
/*
	dev->cport   = iobase+1;
	dev->rport   = iobase+2;
	dev->wport   = iobase+3;
	dev->wtcport = iobase+0;
	dev->irq     = irq;
	dev->PlxIntCsrPort = 0;
*/
	dev->ctl = 0x0F;
	SETCTL;
	SETCTL;
	dev->ctl = 0x0D;
	SETCTL;
	SETCTL;
	dev->ctl |= irq_table[dev->irq];
	SETCTL;
	SETCTL;
	dev->ctl &= 0xF6;
	SETCTL;

	if( request_irq(irq, lpc_isa_interrupt, SA_SHIRQ, DRIVER_DEV_NAME, dev) ) 
        {
		err("unable to register IRQ%d\n", irq);
#ifndef CHECK_ISA_AT_LOAD_TIME
		release_region(iobase, 4);
#endif
		return -ENODEV;
        }
	return 0;
    }

#ifndef NO_PCI
static int initialize_pcicard( DEV *dev, int index )
    {
    dbg1("%s(): dev=%08X", __FUNCTION__, (uint)dev);
        {    
#if 0
        u32  PlxIntCsrPort, cport, rport, wtcport, rtr;
        int  irq;
        char bus, function;
        
        if( !pcibios_present() )
            {
            err( "PCI bios not present\n" );
            return -ENODEV;
            }
        if( pcibios_find_device( VENDOR_ID, DEVICE_ID, index, &bus, &function ) )
            {
            err( "PCI device not found\n" );
            return -ENODEV;
            }
        pcibios_read_config_dword( bus, function, 0x14, (int *)&rtr );
        pcibios_read_config_dword( bus, function, 0x18, (int *)&cport );
        pcibios_read_config_dword( bus, function, 0x1C, (int *)&rport );
        pcibios_read_config_dword( bus, function, 0x24, (int *)&wtcport );
        pcibios_read_config_dword( bus, function, 0x3C, &irq );
        PlxIntCsrPort = (rtr & 0xFFF0) + P9050_INTCSR;
        cport   &= 0xFFF0;
        rport   &= 0xFFF0;
        irq     &= 0x0F;
        outw( 0, PlxIntCsrPort );
        
        if( check_region(cport, 4) ) {
            err( "unable to register port 0x%X\n", cport );
            return -ENODEV;
            }
        if( check_region(rport, 4) ) {
            err( "unable to register port 0x%X\n", rport );
            return -ENODEV;
            }
        request_region( cport, 4, DRIVER_DEV_NAME );
        request_region( rport, 4, DRIVER_DEV_NAME );
        dev->cport   = cport;
        dev->rport   = rport;
        dev->wport   = rport;
        dev->wtcport = wtcport;
        dev->irq     = irq;
        dev->PlxIntCsrPort = PlxIntCsrPort;
#else
        u32  cport;
        u16  IntCsr;
        cport   = dev->cport;
        
#endif
        dev->ctl = 0x0D;
        SETCTL;
        SETCTL;
        dev->ctl = 0x16;
        SETCTL;
        SETCTL;
        dev->ctl = 0x02;
        SETCTL;
        SETCTL;
        dev->ctl = 0x16;
        SETCTL;
        IntCsr = inw(dev->PlxIntCsrPort);
        dbg1("? %s(): PlxIntCsrPort=%04X IntCsr=%04X", __FUNCTION__, dev->PlxIntCsrPort, IntCsr);
        IntCsr |= PLX_9050_INTR_ENABLE | PLX_9050_LINT1_ENABLE; // | PLX_9050_LINT2_ENABLE;
        outw(IntCsr, dev->PlxIntCsrPort);
        dbg1("? %s(): PlxIntCsrPort=%04X IntCsr=%04X", __FUNCTION__, dev->PlxIntCsrPort, IntCsr);
        
        if( request_irq(dev->irq, lpc_pci_interrupt, SA_SHIRQ, DRIVER_DEV_NAME, dev) ) 
            {
            err("unable to register IRQ%d\n", dev->irq);
#if 0
            release_region(rport, 4);
            release_region(cport, 4);
#endif
            return -ENODEV;
            }
        return 0;
        }
    }
#endif  // NO_PCI

static int lpc_open(struct inode *inode, struct file *file)
    {
	int minor;
//	u32 cport;
	int idx;
	register DEV *dev;

    dbg1("%s(): MINOR=%d", __FUNCTION__, MINOR (inode->i_rdev));
/*
    DbgFlush();
    struct timeval tv; 
    do  {
        do_gettimeofday(&tv);
        } while ( (tv.tv_sec % 10) != 0);
*/
	minor = iminor(inode);
	if( minor > MAX_DEVICES ) 
        {
		err("%s - invalid minor number %d", __FUNCTION__, minor);
		return -ENODEV;
        }
    dev = minor_table[minor];
	if( !dev ) 
        {
		err ("%s - error, can't find device for minor %d", __FUNCTION__, minor);
		return -ENODEV;
        }

    /* increment our usage count for the driver */
    if (++dev->open_count == 1)
        {
        dev->active = 1;
        
        if( minor < MAX_LPCS ) 
            {
            if( initialize_isacard(dev, minor) ) 
                {
                /* decrement our usage count for the device */
                --dev->open_count;
                return -ENODEV;
                }
            }
        else 
            {
#ifndef NO_PCI
            if( initialize_pcicard(dev, minor - MAX_LPCS) ) 
#endif
                {
                /* decrement our usage count for the device */
                --dev->open_count;
                return -ENODEV;
                }
            }
    
//        dev->PciIntCount = dev->PciIntCountPlus = dev->PciIntCountMinus = 0;
//        dev->TimerCountPlus = dev->TimerCountMinus = 0;

#if LINUX_VERSION_CODE < 0x020400
        dev->rd_wait    = NULL;
//	    dev->wr_wait    = NULL;
#else
//  #ifdef DECLARE_WAITQUEUE
        init_waitqueue_head(&dev->rd_wait);
//      dbg8("%s(): init_waitqueue_head()", __FUNCTION__);
//	    init_waitqueue_head(&dev->wr_wait);
//  #endif
#endif
        dev->inhead  = dev->intail = 0;
        dev->outhead = dev->outtail = 0;
        dev->state   = ST_RESYNC;
        dev->active  = 0;
        dev->wtc_exist = 0;

        init_timer( &dev->drvtimer );
        dev->drvtimer.function = lpc_timer;
        dev->drvtimer.data = (unsigned long)dev;
//        cport = dev->cport;
        
#if LINUX_VERSION_CODE < 0x020500
        MOD_INC_USE_COUNT;
#endif
        dbg1("%s(): before sm_stub()", __FUNCTION__);
        sm_stub( dev );                             // Start state machine
#if 01
  #if 0
        dbg8("%s(): before interruptible_sleep_on()", __FUNCTION__);
        interruptible_sleep_on( &dev->rd_wait );    // and read 1st reset msg
        dbg8("%s(): after interruptible_sleep_on()", __FUNCTION__);
        idx = dev->inhead;
  #else
        idx = dev->inhead;
//    if (qempty(&dev->ReadBufferQueue))
        if (idx == dev->intail)
            {
            dbg1("%s(): qempty", __FUNCTION__);
//        up (&dev->sem);

//        if (wait_event_interruptible(dev->rd_wait, qhasdata(&dev->ReadBufferQueue)))
            if (wait_event_interruptible(dev->rd_wait, (idx != dev->intail) ))
                {
                /* decrement our usage count for the device */
                --dev->open_count;
                return -ERESTARTSYS; /* signal: tell the fs layer to handle it */
                }
            dbg8("%s(): nach wait_event_interruptible(rd_wait)", __FUNCTION__);
//        if (down_interruptible(&dev->sem))
//            return -ERESTARTSYS;
            }
  #endif
        idx++;
        if(idx >= NUM_INBUF) idx = 0;
        dev->inhead = idx;
#endif
        }
    /* save our object in the file's private structure */
	file->private_data = dev;

    dbg1("%s(): return 0", __FUNCTION__);
	return 0;
    }


static int lpc_release(struct inode *inode, struct file *file)
    {
    register DEV *dev = (DEV *)(file->private_data);
    if (dev == NULL) 
        {
        dbg1("%s() dev=NULL", __FUNCTION__);
        return -ENODEV;
        }
    
    dbg1("%s(): minor=%d open_count=%d", __FUNCTION__, dev->minor, dev->open_count);
/*
    dbg("PciIntCount=%5d PciIntCountPlus=%5d PciIntCountMinus=%5d"
        "\nTimerCountPlus=%5d TimerCountMinus=%5d",
        dev->PciIntCount, dev->PciIntCountPlus, dev->PciIntCountMinus,
        dev->TimerCountPlus, dev->TimerCountMinus);
    DbgFlush();
*/
    if (dev->open_count <= 0) 
        {
        dbg1("%s() device not opened", __FUNCTION__);
        return -ENODEV;
        }
    
    /* decrement our usage count for the device */
    if (--dev->open_count <= 0) 
        {
        u32 cport = dev->cport;
        dev->active = 1;
        dbg8("%s(): before del_timer()", __FUNCTION__);
        del_timer( &dev->drvtimer );
        free_irq(dev->irq, dev);
        dbg8("%s(): after free_irq()", __FUNCTION__);
        dev->ctl = 0x0D;
        SETCTL;
        if (dev->PlxIntCsrPort)
            {
            u16 IntCsr = inw(dev->PlxIntCsrPort);
            dbg1("? %s(): PlxIntCsrPort=%04X IntCsr=%04X", __FUNCTION__, dev->PlxIntCsrPort, IntCsr);
            IntCsr &= ~PLX_9050_INTR_ENABLE;
            outw(IntCsr, dev->PlxIntCsrPort);
            dbg1("? %s(): PlxIntCsrPort=%04X IntCsr=%04X", __FUNCTION__, dev->PlxIntCsrPort, IntCsr);
//            release_region(dev->cport, 4);
//            release_region(dev->rport, 4);
            }
        else
            {
            outw( 0, dev->rport );
#ifndef CHECK_ISA_AT_LOAD_TIME
            release_region(cport-1, 4);
#endif
            }
//        devs[minor] = NULL;
//        kfree( dev );
        dev->open_count = 0;
        }
    file->private_data = 0;
    
#if LINUX_VERSION_CODE < 0x020500
    MOD_DEC_USE_COUNT;
#endif
    dbg1("%s(): return 0", __FUNCTION__);
//    DbgFlush();
    return 0;
    }


int lpc_watcher( DEV* dev, u8* buf )
{
	u32 cport, wtcport;
	u8  *s, *d;
	s16 count, i;
	u8  xcvr, cmd, tmp;
	int t;

    dbg1("%s(): dev=%08X", __FUNCTION__, (uint)dev);
    
	cport   = dev->cport;
	wtcport = dev->wtcport;

	s = d = buf;
#if 01
	__get_user(count, s++);					// Get the length
	__get_user(tmp, s++);
	count = (count << 8) + tmp;
	__get_user(cmd, s++);
	cmd &= 0x7F;
#else
	count = *s++;					// Get the length
	count = (count << 8) + *s++;
	cmd = *s++ & 0x7F;
#endif

	if( cmd ) 
    {				// Normal case: request watcher firmware
		if(!dev->wtc_exist) return -ENODEV;
		t = jiffies + HZ/2;
		WTCHS;
		WOUT((u8)(count >> 8));		// Send the length
		WTCHS;
		WOUT((u8)(count & 0xFF));
		WTCHS;
		WOUT(cmd);
		count--;
		while( count > 0 ) {
			WTCHS;
#if 01
            __get_user(tmp, s++);
			WOUT(tmp);					// Send the data bytes
#else
			WOUT(*s++);					// Send the data bytes
#endif
			count--;
		}
		WTCHS;
		WOUT(0);						   // EOM

		WTCHS;
		count = WIN;					// Receive the length
#if 01
        __put_user(count & 0xFF, d++);
		WTCHS;
		count = (count << 8) + WIN;
        __put_user(count & 0xFF, d++);
#else
		*d++ = count & 0xFF;
		WTCHS;
		count = (count << 8) + WIN;
		*d++ = count & 0xFF;
#endif

		i = count;
		while( i > 0 ) {
			WTCHS;
#if 01
			tmp = WIN;
            __put_user(tmp, d++);
#else
			*d++ = WIN;
#endif
			i--;
		}

	} else {				// Special case: firmware download

		dev->ctl |= 0x02;					// Reset LWA
		SETCTL;
		WIN;							// Switch to Bootloader
		dev->ctl &= ~0x02;				// Start LWA
		SETCTL;
#if 01
        __get_user(xcvr, s++);
#else
        xcvr = *s++;
#endif
		count -=2;

		schedule();
		t = jiffies + HZ;
		while( count > 0 ) {
			WTCHS;
#if 01
            __get_user(tmp, s++);
			WOUT(tmp);					// Send the data bytes
#else
			WOUT(*s++);					// Send the data bytes
#endif
			count--;
		}
		WTCHS;
		WOUT(0);						// EOM
		WTCHS;							// Wait until finished

		dev->ctl |= 0x02;
		SETCTL;							// Reset LWA
		WOUT(xcvr);						// Switch to Firmware
		dev->ctl &= ~0x02;
		SETCTL;							// Start LWA

		t = HZ * 2;						// Give the LWA some time to answer
		do {
			t--;
			if( !t ) return -ENODEV;
			schedule();
		}
		while(LPCSTAT & 2);
#if 01
        __put_user(0, d++);
        __put_user(0, d++);
#else
		*d++ = 0;						// Acknowledge
		*d++ = 0;
#endif
		dev->wtc_exist = 1;
	}
	return d - buf;
}



static int lpc_ioctl(struct inode *inode, struct file *file,
					 unsigned int cmd, unsigned long arg)
{
    int retval = -EINVAL;
	register DEV *dev = (DEV *)(file->private_data);
    char tmpBuf[80];

    dbg1("%s(): dev=%08X", __FUNCTION__, (uint)dev);
    
	switch( cmd )
	{
		case IOCTL_VERSION:
            retval = sprintf(tmpBuf, DRIVER_DESC " " DRIVER_VERSION ", Linux %d.%d.%d",
							(LINUX_VERSION_CODE >> 16) & 0xFF,
							(LINUX_VERSION_CODE >> 8) & 0xFF,
							LINUX_VERSION_CODE & 0xFF ) + 1;
            if (copy_to_user ((char *)arg, tmpBuf, retval))
                retval = -EFAULT;
            break;
		case IOCTL_WATCHER:
			return lpc_watcher( dev, (u8 *)arg );
	}
	return retval;
}



int lpc_init( void )
    {
    int i;
    u32 iobase, cport, rport;
    int irq;
    int rc = -ENODEV;
    
    dbg1("%s():", __FUNCTION__);
    
    for( i=0; i<MAX_LPCS; i++ )
        {
        if( !lpcs[i] ) continue;
        
        iobase = lpcs[i] & 0x0FFF;
        irq    = lpcs[i] >> 12;
        cport = iobase+1;
        rport = iobase+2;
        dbg1("lpc[%2d]: iobase=%04X irq=%X", i, iobase, irq);
#ifdef CHECK_ISA_AT_LOAD_TIME
        if( ((u8)irq > 15) || !irq_table[irq] ) {
            err( "IRQ%d not supported\n", irq );
            lpcs[i] = 0;
            continue;
            }
        if (!request_region( iobase, 4, DRIVER_DEV_NAME ))
            // if( check_region(iobase, 4) )
            {
            err("unable to register port 0x%X", iobase);
            lpcs[i] = 0;
            continue;
            }

            {
            int hwfail = 0;

            outb(0x5F, cport);
            outb(0, rport);
            outb(0, rport);
            outb(0, rport);
            if( (inb(cport) & 0xF0) != 0x50 )  hwfail = 1;
            outb(0x0F, cport);
            outb(0, rport);
            outb(0, rport);
            outb(0, rport);
            if( inb(cport) & 0xF0 )  hwfail = 1;
            if( hwfail ) 
                {
                err( "interface card not responding at port 0x%04X", iobase );
                lpcs[i] = 0;
                release_region( iobase, 4);
                continue;
                }
            // request_region( iobase, 4, DRIVER_DEV_NAME );
            }
#endif
            {
            DEV *dev = NULL;
            uint minor;
            
            /* select a "subminor" number (part of a minor number) */
            down (&minor_table_mutex);
            for (minor = 0; minor < MAX_LPCS; ++minor) 
                {
                if (minor_table[minor] == NULL)
                    break;
                }
            if (minor >= MAX_LPCS) 
                {
                info ("Too many devices plugged in, can not handle this device.");
                rc = -EINVAL;
                goto err_minor_table;
                }
            
            dev = kmalloc (sizeof(DEV), GFP_KERNEL);
            if (!dev)  
                {
                up (&minor_table_mutex);
#ifdef CHECK_ISA_AT_LOAD_TIME
                release_region( iobase, 4);
#endif
                return -ENOMEM;
                }
            memset (dev, 0x00, sizeof (*dev));
            
            dev->cport   = iobase+1;
            dev->rport   = iobase+2;
            dev->wport   = iobase+3;
            dev->wtcport = iobase+0;
            dev->irq     = irq;
            dev->PlxIntCsrPort = 0;

            dev->minor = minor;
            minor_table[minor] = dev;
            up (&minor_table_mutex);
            
            rc = 0;     // found on or more devices
            }
        }
    return rc;

err_minor_table:
    up (&minor_table_mutex);
#ifdef CHECK_ISA_AT_LOAD_TIME
    release_region( iobase, 4);
#endif
    return rc;
    }


#include "driverentry.c"
