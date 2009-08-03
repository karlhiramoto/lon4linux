#ifndef _KDEBUG_
#define _KDEBUG_


#ifdef __cplusplus
extern "C" {
#endif

#define DEBUG_DUMP1             0x0001
#define DEBUG_DUMP2             0x0002
#define DEBUG_DUMP_WATCHER      0x0004
#define DEBUG_OPEN_CLOSE        0x0008
#define DEBUG_OPEN_IOCTL        0x0010
#define DEBUG_REGISTER          0x0100
#define DEBUG_DISPATCH_READ     0x0200
#define DEBUG_DISPATCH_WRITE    0x0400

#if 0
// queue.c  queuec.c queuei.c queued.c
struct tomqueue
    {
    uint datalen;
    uint wrp;         /* write pointer    */
    uint rdp;         /* read pointer     */
    uint count;       /* number of free chars */

//    uint stop:1;	    /* nicht verwendet  */
//    uint xoffsend:1;    /* XON / XOFF gesendet */
//    uint reserved:14;

    unsigned char data[1];      /* raw data         */
    } ;
typedef struct tomqueue * QUEUE;
#define sizeofQUEUE (sizeof(struct tomqueue))
#define qempty(QUEUE) (QUEUE->wrp == QUEUE->rdp)
#define qcontent(QUEUE) (QUEUE->datalen - QUEUE->count)
#define QSIZE(QUEUE) (QUEUE->datalen)
#define qfree(QUEUE) (QUEUE->count)

void  qflush (QUEUE qptr);
int   qwrite (QUEUE qptr,void* buffer,int anzbytes);
int   qwritec(QUEUE qptr,char writeval);
int   qwritei(QUEUE qptr,int  writeval);
int   qwrited(QUEUE qptr,void *ptr);
int   qread  (QUEUE qptr,void* buffer,int anzbytes);
int   qreadc (QUEUE qptr);
int   qreadi (QUEUE qptr);
void *qreadd (QUEUE qptr);

// queueopn.c  queueshm.c
QUEUE qopen     (int queue_length_in_bytes);
QUEUE qopen_shm (int queue_length_in_bytes);
void  qclose    (QUEUE qptr);

// queuestr.c
int qwritestr(QUEUE q, char *buff);
int qreadstr(QUEUE q, char *buff);



int DbgInit(void);
void DbgExit(void);
void DbgFlush(void);

asmlinkage int DbgPrint(const char *fmt, ...);
#endif

/* Use our own dbg macro */
#undef dbg
#define dbg(format, arg...) do { if (debug) { \
    struct timeval tv; do_gettimeofday(&tv); \
    printk(KERN_DEBUG        /*__FILE__*/ DRIVER_MOD_NAME "%2u.%06u: " format "\n",        (int)(tv.tv_sec % 10), (int)tv.tv_usec, ## arg); } } while (0)

#if 01
  #define dbg1(format, arg...) do { if (debug & 0x10) dbg(format, ## arg); } while (0)
  #define dbg2(format, arg...) do { if (debug & 0x20) dbg(format, ## arg); } while (0)
  #define dbg4(format, arg...) do { if (debug & 0x40) dbg(format, ## arg); } while (0)
  #define dbg8(format, arg...) do { if (debug & 0x80) dbg(format, ## arg); } while (0)
#else
  #define dbg1(format, arg...) do { if (debug & 0x01) printk(KERN_DEBUG __FILE__ ": " format "\n" , ## arg); } while (0)
  #define dbg2(format, arg...) do { if (debug & 0x02) printk(KERN_DEBUG __FILE__ ": " format "\n" , ## arg); } while (0)
  #define dbg4(format, arg...) do { if (debug & 0x04) printk(KERN_DEBUG __FILE__ ": " format "\n" , ## arg); } while (0)
  #define dbg8(format, arg...) do { if (debug & 0x08) printk(KERN_DEBUG __FILE__ ": " format "\n" , ## arg); } while (0)
#endif


inline void DumpBuffer(PCHAR Prefix, PVOID pvBuffer, ULONG length)
//static inline void lonusb_debug_data (const char *function, int size, const unsigned char *data)
    {
    int i;
    unsigned char *data = pvBuffer;

//    if (!debug)  return;

    printk (KERN_DEBUG "%s ", Prefix);
    for (i = 0; i < length; ++i) 
        {
        printk ("%.2X ", data[i]);
        }
    printk ("\n");
    }


#ifdef __cplusplus
	}
#endif


#endif // _KDEBUG_
