#ifndef _PLXLIB_H_
#define _PLXLIB_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
#ifndef _NTDEF_
  typedef BOOL BOOLEAN;
#endif
*/

/*
// PCI register definitions 
enum {
    PCI_IDR  = 0x00,
    PCI_CR   = 0x04,
    PCI_SR   = 0x06,
    PCI_REV  = 0x08,
    PCI_CCR   = 0x09,
    PCI_LSR  = 0x0c,
    PCI_LTR  = 0x0d,
    PCI_HTR  = 0x0e,
    PCI_BISTR= 0x0f,
    PCI_BAR0 = 0x10,
    PCI_BAR1 = 0x14,
    PCI_BAR2 = 0x18,
    PCI_BAR3 = 0x1c,
    PCI_BAR4 = 0x20,
    PCI_BAR5 = 0x24,
    PCI_CIS  = 0x28,
    PCI_SVID = 0x2c,
    PCI_SID  = 0x2e,
    PCI_ERBAR= 0x30,
    PCI_ILR  = 0x3c,
    PCI_IPR  = 0x3d,
    PCI_MGR  = 0x3e,
    PCI_MLR  = 0x3f
};
*/

// PLX register definitions 
enum {
    P9050_LAS0RR  = 0x00,
    P9050_LAS1RR  = 0x04,
    P9050_LAS2RR  = 0x08,
    P9050_LAS3RR  = 0x0c,
    P9050_EROMRR  = 0x10,
    P9050_LAS0BA  = 0x14,
    P9050_LAS1BA  = 0x18,
    P9050_LAS2BA  = 0x1c,
    P9050_LAS3BA  = 0x20,
    P9050_EROMBA  = 0x24,
    P9050_LAS0BRD = 0x28,
    P9050_LAS1BRD = 0x2c,
    P9050_LAS2BRD = 0x30,
    P9050_LAS3BRD = 0x34,
    P9050_EROMBRD = 0x38,
    P9050_CS0BASE = 0x3c,
    P9050_CS1BASE = 0x40,
    P9050_CS2BASE = 0x44,
    P9050_CS3BASE = 0x48,
    P9050_INTCSR  = 0x4c,
    P9050_CNTRL   = 0x50
};

/****** Data Structures *****************************************************/
/*
 *      RUNTIME_9050 - PLX PCI9050-1 local configuration and shared runtime
 *      registers. This structure can be used to access the 9050 registers
 *      (memory mapped).
 */
/*
typedef struct _PLX_9050
    {
    ULONG  loc_addr_range[4];      // 00-0Ch : Local Address Ranges
    ULONG  loc_rom_range;          // 10h : Local ROM Range
    ULONG  loc_addr_base[4];       // 14-20h : Local Address Base Addrs
    ULONG  loc_rom_base;           // 24h : Local ROM Base
    ULONG  loc_bus_descr[4];       // 28-34h : Local Bus Descriptors
    ULONG  rom_bus_descr;          // 38h : ROM Bus Descriptor
    ULONG  cs_base[4];             // 3C-48h : Chip Select Base Addrs
    ULONG  intr_ctrl_stat;         // 4Ch : Interrupt Control/Status
    ULONG  init_ctrl;              // 50h : EEPROM ctrl, Init Ctrl, etc
    } PLX_9050, *PPLX_9050;
*/
/*
4.3.20 (INTCSR; 4Ch) Interrupt Control/Status Register
Talbe 43: Interrupt Control/Status Register Description
Field Description							                                     Read Write	Value after Reset
0	  Local interrupt 1 enable.   1 = Enabled. 0 = Disabled.			         Yes  Yes	0
1	  Local interrupt 1 polarity. 1 = Active high. 0 = Active low.		         Yes  Yes	0
2	  Local interrupt 1 status.   1 = Interrupt active. 0 = Interrupt not active Yes  No	0
3	  Local interrupt 2 enable.   1 = Enabled. 0 = Disabled.			         Yes  Yes	0
4	  Local interrupt 2 polarity. 1 = Active high. 0 = Active low.		         Yes  Yes	0
5	  Local interrupt 2 status.   1 = Interrupt active. 0 = Interrupt not active Yes  No	0
6	  PCI interrupt enable. A value of 1 will enable PCI interrupt.		         Yes  Yes	0
7	  Software Interrupt.         1 = Generate interrupt.				         Yes  Yes	0
31:8  Not used.                                                                  Yes  No	0
*/

#define PLX_9050_LINT1_ENABLE   0x01
#define PLX_9050_LINT1_POL      0x02
#define PLX_9050_LINT1_STATUS   0x04
#define PLX_9050_LINT2_ENABLE   0x08
#define PLX_9050_LINT2_POL      0x10
#define PLX_9050_LINT2_STATUS   0x20
#define PLX_9050_INTR_ENABLE    0x40
#define PLX_9050_SW_INTR        0x80



typedef enum
{
    P9050_MODE_BYTE   = 0,
    P9050_MODE_WORD   = 1,
    P9050_MODE_ULONG  = 2
} P9050_MODE;

typedef enum
{
    P9050_ADDR_REG     = 0,
    P9050_ADDR_SPACE0  = 1,
    P9050_ADDR_SPACE1  = 2,
    P9050_ADDR_SPACE2  = 3,
    P9050_ADDR_SPACE3  = 4,
    P9050_ADDR_EPROM   = 5
} P9050_ADDR;


enum { P9050_RANGE_REG = 0x00000080 };
/*
typedef struct _P9050_ADDR_DESC
{
    ULONG dwLocalBase;
    ULONG dwMask;
    ULONG dwBytes;
    ULONG dwAddr;
    ULONG dwAddrDirect;
    BOOLEAN  fIsMemory;
    ULONG modeDesc[3];
} P9050_ADDR_DESC;
*/
/*
typedef struct
{
    HANDLE hWD;
    WD_CARD cardLock;
    WD_INTERRUPT Int;
    WD_PCI_SLOT pciSlot;
    WD_CARD_REGISTER cardReg;
    HANDLE hIntThread;
    P9050_ADDR_DESC addrDesc[6];
    BOOL   fUseInt;
    WD_TRANSFER IntTrans[2];
} P9050_STRUCT, *P9050HANDLE;
*/
/*
typedef struct
{
    ULONG dwCounter;      // number of interrupts received
    ULONG dwLost;         // number of interrupts not yet dealt with
    BOOLEAN  fStopped;       // was interrupt disabled during wait
    ULONG dwIntStatusReg; // value of status register when interrupt occured
} P9050_INTERRUPT;
*/

/*
// options for PLX_Open
enum { P9050_OPEN_USE_INT =   0x1 };

ULONG P9050_CountCards (ULONG dwVendorID, ULONG dwDeviceID);
BOOL P9050_Open (P9050HANDLE *phPlx, ULONG dwVendorID, ULONG dwDeviceID, ULONG nCardNum, ULONG options);
void P9050_Close (P9050HANDLE hPlx);

BOOL P9050_IsAddrSpaceActive(P9050HANDLE hPlx, P9050_ADDR addrSpace);
void P9050_ReadBlock (P9050HANDLE hPlx, ULONG dwLocalAddr, PVOID buf, 
                    ULONG dwBytes, P9050_ADDR addrSpace, P9050_MODE mode);
void P9050_WriteBlock (P9050HANDLE hPlx, ULONG dwLocalAddr, PVOID buf, 
                     ULONG dwBytes, P9050_ADDR addrSpace, P9050_MODE mode);
BYTE P9050_ReadByte (P9050HANDLE hPlx, P9050_ADDR addrSpace, ULONG dwLocalAddr);
void P9050_WriteByte (P9050HANDLE hPlx, P9050_ADDR addrSpace, ULONG dwLocalAddr, BYTE data);
WORD P9050_ReaULONG (P9050HANDLE hPlx, P9050_ADDR addrSpace, ULONG dwLocalAddr);
void P9050_WriteWord (P9050HANDLE hPlx, P9050_ADDR addrSpace, ULONG dwLocalAddr, WORD data);
ULONG P9050_ReadULONG (P9050HANDLE hPlx, P9050_ADDR addrSpace, ULONG dwLocalAddr);
void P9050_WriteULONG (P9050HANDLE hPlx, P9050_ADDR addrSpace, ULONG dwLocalAddr, ULONG data);

BOOL P9050_IntEnable(P9050HANDLE hPlx);
void P9050_IntDisable(P9050HANDLE hPlx);
void P9050_IntWait(P9050HANDLE hPlx, P9050_INTERRUPT *intResult);

void P9050_WriteReg (P9050HANDLE hPlx, ULONG dwReg, ULONG dwData);
ULONG P9050_ReadReg (P9050HANDLE hPlx, ULONG dwReg);
*/


enum 
{
    BIT0  = 0x00000001,
    BIT1  = 0x00000002,
    BIT2  = 0x00000004,
    BIT3  = 0x00000008,
    BIT4  = 0x00000010,
    BIT5  = 0x00000020,
    BIT6  = 0x00000040,
    BIT7  = 0x00000080,
    BIT8  = 0x00000100,
    BIT9  = 0x00000200,
    BIT10 = 0x00000400,
    BIT11 = 0x00000800,
    BIT12 = 0x00001000,
    BIT13 = 0x00002000,
    BIT14 = 0x00004000,
    BIT15 = 0x00008000,
    BIT16 = 0x00010000,
    BIT17 = 0x00020000,
    BIT18 = 0x00040000,
    BIT19 = 0x00080000,
    BIT20 = 0x00100000,
    BIT21 = 0x00200000,
    BIT22 = 0x00400000,
    BIT23 = 0x00800000,
    BIT24 = 0x01000000,
    BIT25 = 0x02000000,
    BIT26 = 0x04000000,
    BIT27 = 0x08000000,
    BIT28 = 0x10000000,
    BIT29 = 0x20000000,
    BIT30 = 0x40000000,
    BIT31 = 0x80000000
};

#ifdef __cplusplus
}
#endif

#endif
