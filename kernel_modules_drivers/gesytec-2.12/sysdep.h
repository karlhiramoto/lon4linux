#ifndef _SYSDEP_
#define _SYSDEP_

#ifdef __cplusplus
extern "C" {
#endif

//===================================================================
// some Windows types
//===================================================================

#ifndef CONST
#define CONST               const
#endif

// Basics
#ifndef VOID
#define VOID void
typedef void *PVOID;

typedef char CHAR;
typedef short SHORT;
typedef long LONG;
#endif


// Pointer to Basics
typedef CHAR *PCHAR;
typedef CHAR *LPSTR, *PSTR;
typedef CONST CHAR *LPCSTR, *PCSTR;
typedef SHORT *PSHORT;  // winnt
typedef LONG *PLONG;    // winnt

// Unsigned Basics
typedef unsigned char BYTE;
typedef unsigned char UCHAR;
typedef unsigned short USHORT;
typedef unsigned long ULONG;

// Pointer to Unsigned Basics
typedef UCHAR *PUCHAR;
typedef USHORT *PUSHORT;
typedef ULONG *PULONG;

typedef UCHAR BOOLEAN;           // winnt
typedef BOOLEAN *PBOOLEAN;       // winnt
#ifndef FALSE
  #define FALSE   0
#endif
#ifndef TRUE
  #define TRUE    1
#endif


//typedef LONG NTSTATUS, *PNTSTATUS;
typedef int NTSTATUS, *PNTSTATUS;

#ifndef min
  #define min(a,b) ((a) > (b) ? (b) : (a))
#endif



//===================================================================
// Compatibility with older Linux versions
//===================================================================

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
// Kernel 2.4

#define irqreturn_t void 
#define IRQ_HANDLED
#define IRQ_NONE

#define iminor(inode) MINOR(inode->i_rdev)
#define pci_name(pdev) (pdev->slot_name)
#define pm_message_t u32

#define USB_SUBMIT_URB(urb, mem_flags) usb_submit_urb(urb)

#else
// Kernel 2.6

#define USB_SUBMIT_URB(urb, mem_flags) usb_submit_urb(urb, mem_flags)


/* Basic class macros */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15)
  typedef struct class class_t;
  #define CLASS_DEVICE_CREATE(cls, devt, device, fmt, arg...) class_device_create(cls, NULL, devt, device, fmt, ## arg)
#else /* LINUX 2.6.0 - 2.6.14 */
  #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13) /* LINUX 2.6.13 - 2.6.14 */
    typedef struct class class_t;
    #define CLASS_DEVICE_CREATE class_device_create
  #else /* LINUX 2.6.0 - 2.6.12, class_simple */
    typedef struct class_simple class_t;
    #define CLASS_DEVICE_CREATE class_simple_device_add
    #define class_create class_simple_create
    #define class_destroy class_simple_destroy
    #define class_device_destroy(a, b) class_simple_device_remove(b)
  #endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13) */
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15) */

#endif


#ifdef __cplusplus
	}
#endif


#endif // _SYSDEP_
