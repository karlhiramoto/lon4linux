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
  #define TRUE    1
#endif



//===================================================================
// Compatibility with older Linux versions
//===================================================================

#if LINUX_VERSION_CODE < 0x020500

#define irqreturn_t void 
#define IRQ_HANDLED
#define IRQ_NONE

#define iminor(inode) MINOR(inode->i_rdev)

#endif


#ifdef __cplusplus
	}
#endif


#endif // _SYSDEP_
