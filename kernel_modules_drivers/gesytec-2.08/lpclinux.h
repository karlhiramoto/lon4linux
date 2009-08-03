/*****************************************************************************/
/*
 * Linux driver for Easylon ISA/PC-104/PCI Interfaces
 *
 * Copyright (c)2004 Gesytec GmbH, Volker Schober (info@gesytec.de)
 * Licensend for use with Easylon USB Interfaces by Gesytec
 *
 * Please compile lpclinux.c
 *
 */
 /*****************************************************************************/

#include <linux/module.h>
#include <linux/config.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#include <linux/fs.h>
#include <linux/major.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
  #include <linux/workqueue.h>
#else
  #include <linux/tqueue.h>
#endif
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/timer.h>
#include <linux/poll.h>
#include <linux/pci.h>
#include <linux/usb.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
  #include <linux/moduleparam.h>
  #include <linux/interrupt.h>
  #include <linux/devfs_fs_kernel.h>
#else
  #include <asm/irq.h>
#endif
#include <asm/io.h>

#include "sysdep.h"
#include "kdebug.h"
#include "Plx9050.h"
//#include "dynbuf.h"



#define CHECK_ISA_AT_LOAD_TIME


/* Version Information */
#define DRIVER_VERSION  "V2.08a"
#define DRIVER_AUTHOR   "Volker Schober, vschober@gesytec.de"
#define DRIVER_DESC     "Easylon LPC/LPP PnP Driver"
#define DRIVER_MOD_NAME "lpclinux"  /* module name */
#define DRIVER_DEV_NAME "lpc"  /* /dev/lonusb */


/* Define these values to match your device */
#define GESYTEC_VENDOR_ID     0x1555
#define LPP_PRODUCT_ID        0x0002

#define IOCTL_VERSION   0x43504C00
#define IOCTL_WATCHER   0x43504C01


#define PFX			DRIVER_MOD_NAME ": "

#define NUM_INBUF   64
#define NUM_OUTBUF  16
#define MAX_LPCS    32
#define MAX_LPPS    32

#define MAX_DEVICES (MAX_LPCS + MAX_LPPS)


typedef struct _DEVICE_EXTENSION {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
    struct wait_queue *rd_wait;
//    struct wait_queue *wr_wait;
#else
    wait_queue_head_t rd_wait;
//    wait_queue_head_t wr_wait;
#endif
    uint                    minor;
    int                     open_count;             /* number of times this port has been opened */
/*
    int     PciIntCount;
    int     PciIntCountPlus;
    int     PciIntCountMinus;

    int     TimerCountPlus;
    int     TimerCountMinus;
*/
	struct tasklet_struct Dpc;
    struct timer_list drvtimer;

    u32 PlxIntCsrPort;  //pciport;
    u32 cport;
    u32 rport;
    u32 wport;
    u32 wtcport;
    int irq;

    volatile int inhead;
    volatile int intail;
    volatile int outhead;
    volatile int outtail;
    volatile u8 ctl;
    volatile u8 state;
    volatile u8 active;
    u8  wtc_exist;
    u8  inbuf[NUM_INBUF][256];
    u8  outbuf[NUM_OUTBUF][256];
    } DEV, DEVICE_EXTENSION, *PDEVICE_EXTENSION;



#define CMD_NULL		0x00
#define CMD_XFER		0x01
#define CMD_RESYNC		0x5A
#define CMD_ACKSYNC		0x07
#define	EOM				0x00

#define NOUT(x)			outb(x, wport)
#define NIN				inb(rport)
#define LPCSTAT			inb(cport)
#define SETCTL			outb(dev->ctl, cport)
#define HS				while((st = LPCSTAT) & 1) if(!(st & 4)) return -1

#define WOUT(x)			outb(x, wtcport)
#define WIN				inb(wtcport)
#define WTCHS			while(LPCSTAT & 2) if(jiffies>t) return -ENODEV

enum {
	ST_RESYNC = 0,
	ST_RESET,
	ST_FLCAN,
	ST_UPLINK,
	ST_IDLE,
	ST_NIACK,
};


//#define CLI		save_flags(flags); cli()
//#define STI		restore_flags(flags)

/* local function prototypes */
static ssize_t lpc_read        (struct file *file, char *buffer, size_t count, loff_t *ppos);
static ssize_t lpc_write       (struct file *file, const char *buffer, size_t count, loff_t *ppos);
static unsigned lpc_poll       (struct file *file, struct poll_table_struct *wait);
static int lpc_ioctl           (struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);
static int lpc_open            (struct inode *inode, struct file *file);
static int lpc_release         (struct inode *inode, struct file *file);

static int lpp_init_one (struct pci_dev *pdev, const struct pci_device_id *ent);
static void lpp_remove_one (struct pci_dev *pdev);
static int lpp_suspend (struct pci_dev *pdev, u32 state);
static int lpp_resume (struct pci_dev *pdev);
