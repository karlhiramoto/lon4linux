/*
 * ni_msg.h,v 1.0 1996/03/18 14:18:34 miksic Exp
 */

/* NI_MSG.H -- LONWORKS network interface protocol message definitions.
 * Copyright (c) 1993 by Echelon Corporation.
 * All Rights Reserved.
 */

/*
 ****************************************************************************
 * Application buffer structures for sending and receiving messages to and
 * from a network interface.  The 'ExpAppBuffer' and 'ImpAppBuffer'
 * structures define the application buffer structures with and without
 * explicit addressing.  These structures have up to four parts:
 *
 *   Network Interface Command (NI_Hdr)                        (2 bytes)
 *   Message Header (MsgHdr)                                   (3 bytes)
 *   Network Address (ExplicitAddr)                            (11 bytes)
 *   Data (MsgData)                                            (varies)
 *
 * Network Interface Command (NI_Hdr):
 *
 *   The network interface command is always present.  It contains the
 *   network interface command and queue specifier.  This is the only
 *   field required for local network interface commands such as niRESET.
 *
 * Message Header (MsgHdr: union of NetVarHdr and ExpMsgHdr):
 *
 *   This field is present if the buffer is a data transfer or a completion
 *   event.  The message header describes the type of LONTALK message
 *   contained in the data field.
 *
 *   NetVarHdr is used if the message is a network variable message and
 *   network interface selection is enabled.
 *
 *   ExpMsgHdr is used if the message is an explicit message, or a network
 *   variable message and host selection is enabled (this is the default
 *   for the SLTA).
 *
 * Network Address (ExplicitAddr:  SendAddrDtl, RcvAddrDtl, or RespAddrDtl)
 *
 *   This field is present if the message is a data transfer or completion
 *   event, and explicit addressing is enabled.  The network address
 *   specifies the destination address for downlink application buffers,
 *   or the source address for uplink application buffers.  Explicit
 *   addressing is the default for the SLTA.
 *
 *   SendAddrDtl is used for outgoing messages or NV updates.
 *
 *   RcvAddrDtl is used  for incoming messages or unsolicited NV updates.
 *
 *   RespAddrDtl is used for incoming responses or NV updates solicited
 *   by a poll.
 *
 * Data (MsgData: union of UnprocessedNV, ProcessedNV, and ExplicitMsg)
 *
 *   This field is present if the message is a data transfer or completion
 *   event.
 *
 *   If the message is a completion event, then the first two bytes of the
 *   data are included.  This provides the NV index, the NV selector, or the
 *   message code as appropriate.
 *
 *   UnprocessedNV is used if the message is a network variable update, and
 *   host selection is enabled. It consists of a two-byte header followed by
 *   the NV data.
 *
 *   ProcessedNV is used if the message is a network variable update, and
 *   network interface selection is enabled. It consists of a two-byte header
 *   followed by the NV data.
 *
 *   ExplicitMsg is used if the message is an explicit message.  It consists
 *   of a one-byte code field followed by the message data.
 *
 * Note - the fields defined here are for a little-endian (Intel-style)
 * host processor, such as the 80x86 processors used in PC compatibles.
 * Bit fields are allocated right-to-left within a byte.
 * For a big-endian (Motorola-style) host, bit fields are typically
 * allocated left-to-right.  For this type of processor, reverse
 * the bit fields within each byte.  Compare the NEURON C include files
 * ADDRDEFS.H and MSG_ADDR.H, which are defined for the big-endian NEURON
 * CHIP.
 ****************************************************************************
 */


#ifndef _NI_MSG_H
#define _NI_MSG_H	1

#include <stdio.h>

/* Change the following declarations to port to a non-80x86  */

typedef unsigned char byte;     /* 8 bits */
typedef unsigned char bits;          /* bit fields */
typedef unsigned short word;      /* 16 bits */

//typedef enum { FALSE = 0, TRUE } bool;

/*
 ****************************************************************************
 * Network Interface Command data structure.  This is the application-layer
 * header used for all messages to and from a LONWORKS network interface.
 ****************************************************************************
 */

/* Literals for the 'cmd.q.queue' nibble of NI_Hdr. */

typedef enum {
    niTQ          =  2,             /* Transaction queue                    */
    niTQ_P        =  3,             /* Priority transaction queue           */
    niNTQ         =  4,             /* Non-transaction queue                */
    niNTQ_P       =  5,             /* Priority non-transaction queue       */
    niRESPONSE    =  6,             /* Response msg & completion event queue*/
    niINCOMING    =  8              /* Received message queue               */
} NI_Queue;

/* Literals for the 'cmd.q.q_cmd' nibble of NI_Hdr. */

typedef enum {
    niCOMM           = 1,           /* Data transfer to/from network        */
    niNETMGMT        = 2            /* Local network management/diagnostics */
} NI_QueueCmd;

/* Literals for the 'cmd.noq' byte of NI_Hdr. */

typedef enum {
    niNULL           = 0x00,
    niTIMEOUT        = 0x30,        /* Not used                             */
    niCRC            = 0x40,        /* Not used                             */
    niRESET          = 0x50,
    niFLUSH_COMPLETE = 0x60,        /* Uplink                               */
    niFLUSH_CANCEL   = 0x60,        /* Downlink                             */
    niONLINE         = 0x70,
    niOFFLINE        = 0x80,
    niFLUSH          = 0x90,
    niFLUSH_IGN      = 0xA0,
    niSLEEP          = 0xB0,        /* SLTA only                            */
    niACK            = 0xC0,
    niNACK           = 0xC1,        /* SLTA only                            */
    niSSTATUS        = 0xE0,        /* SLTA only                            */
    niPUPXOFF        = 0xE1,
    niPUPXON         = 0xE2,
    niPTRHROTL       = 0xE4,        /* Not used                             */
    niDRV_CMD        = 0xF0,        /* Not used                             */
} NI_NoQueueCmd;

/*
 * Header for network interface messages.  The header is a union of
 * two command formats: the 'q' format is used for the niCOMM and
 * niNETMGMT commands that require a queue specification; the 'noq'
 * format is used for all other network interface commands.
 * Both formats have a length specification where:
 *
 *      length = header (3) + address field (11 if present) + data field
 *
 * WARNING:  The fields shown in this structure do NOT reflect the actual
 * structure required by the network interface.  Depending on the network
 * interface, the network driver may change the order of the data and add
 * additional fields to change the application-layer header to a link-layer
 * header.  See the description of the link-layer header in Chapter 2 of the
 * Host Application Programmer's Guide.
 */

typedef union {
    struct {
        bits queue  :4;         /* Network interface message queue      */
                                /* Use value of type 'NI_Queue'         */
        bits q_cmd  :4;         /* Network interface command with queue */
                                /* Use value of type 'NI_QueueCmd'      */
        bits length :8;         /* Length of the buffer to follow       */
    } q;                        /* Queue option                         */
    struct {
        byte     cmd;           /* Network interface command w/o queue  */
                                /* Use value of type 'NI_NoQueueCmd'    */
        byte     length;        /* Length of the buffer to follow       */
    } noq;                      /* No queue option                      */
} NI_Hdr;

/*
 ****************************************************************************
 * Message Header structure for sending and receiving explicit
 * messages and network variables which are not processed by the
 * network interface (host selection enabled).
 ****************************************************************************
 */

/* Literals for 'st' fields of ExpMsgHdr and NetVarHdr. */

typedef enum {
   ACKD            = 0,
   UNACKD_RPT      = 1,
   UNACKD          = 2,
   REQUEST         = 3
} ServiceType;

/* Literals for 'cmpl_code' fields of ExpMsgHdr and NetVarHdr. */

typedef enum {
    MSG_NOT_COMPL  = 0,             /* Not a completion event               */
    MSG_SUCCEEDS   = 1,             /* Successful completion event          */
    MSG_FAILS      = 2              /* Failed completion event              */
} ComplType;

/* Explicit message and Unprocessed NV Application Buffer. */

typedef struct {

    bits   tag       :4;        /* Message tag for implicit addressing  */
                                /* Magic cookie for explicit addressing */
    bits   auth      :1;        /* 1 => Authenticated                   */
    bits   st        :2;        /* Service Type - see 'ServiceType'     */
    bits   msg_type  :1;        /* 0 => explicit message                */
                                /*      or unprocessed NV               */
/*--------------------------------------------------------------------------*/
    bits   response  :1;        /* 1 => Response, 0 => Other            */
    bits   pool      :1;        /* 0 => Outgoing                        */
    bits   alt_path  :1;        /* 1 => Use path specified in 'path'    */
                                /* 0 => Use default path                */
    bits   addr_mode :1;        /* 1 => Explicit addressing,            */
                                /* 0 => Implicit                        */
                                /* Outgoing buffers only                */
    bits   cmpl_code :2;        /* Completion Code - see 'ComplType'    */
    bits   path      :1;        /* 1 => Use alternate path,             */
                                /* 0 => Use primary path                */
                                /*      (if 'alt_path' is set)          */
    bits   priority  :1;        /* 1 => Priority message                */
/*--------------------------------------------------------------------------*/
    byte   length;              /* Length of msg or NV to follow        */
                                /* not including any explicit address   */
                                /* field, includes code byte or         */
                                /* selector bytes                       */
} ExpMsgHdr;

/*
 ****************************************************************************
 * Message Header structure for sending and receiving network variables
 * that are processed by the network interface (network interface
 * selection enabled).
 ****************************************************************************
 */

typedef struct {
    bits   tag       :4;        /* Magic cookie for correlating         */
                                /* responses and completion events      */
    bits   rsvd0     :2;
    bits   poll      :1;        /* 1 => Poll, 0 => Other                */
    bits   msg_type  :1;        /* 1 => Processed network variable      */
/*--------------------------------------------------------------------------*/
    bits   response  :1;        /* 1 => Poll response, 0 => Other       */
    bits   pool      :1;        /* 0 => Outgoing                        */
    bits   trnarnd   :1;        /* 1 => Turnaround Poll, 0 => Other     */
    bits   addr_mode :1;        /* 1 => Explicit addressing,            */
                                /* 0 => Implicit addressing             */
    bits   cmpl_code :2;        /* Completion Code - see above          */
    bits   path      :1;        /* 1 => Used alternate path             */
                                /* 0 => Used primary path               */
                                /*      (incoming only)                 */
    bits   priority  :1;        /* 1 => Priority msg (incoming only)    */
/*--------------------------------------------------------------------------*/
    byte   length;              /* Length of network variable to follow */
                                /* not including any explicit address   */
                                /* not including index and rsvd0 byte   */
} NetVarHdr;

/* Union of all message headers. */

typedef union {
    ExpMsgHdr  exp;
    NetVarHdr  pnv;
} MsgHdr;

/*
 ****************************************************************************
 * Network Address structures for sending messages with explicit addressing
 * enabled.
 ****************************************************************************
 */

/* Literals for 'type' field of destination addresses for outgoing messages. */

typedef enum {
    UNASSIGNED     = 0,
    SUBNET_NODE    = 1,
    NEURON_ID      = 2,
    BROADCAST      = 3,
    IMPLICIT       = 126,    /* not a real destination type */
    LOCAL          = 127,    /* not a real destination type */
} AddrType;

/* Group address structure.  Use for multicast destination addresses. */

typedef struct {
    bits   size      :7;        /* Group size (0 => huge group)         */
    bits   type      :1;        /* 1 => Group                           */

    bits   rsvd0     :7;
    bits   domain    :1;        /* Domain index                         */

    bits   retry     :4;        /* Retry count                          */
    bits   rpt_timer :4;        /* Retry repeat timer                   */

    bits   tx_timer  :4;        /* Transmit timer index                 */
    bits   rsvd1     :4;

    byte       group;           /* Group ID                             */
} SendGroup;

/* Subnet/node ID address.  Use for a unicast destination address. */

typedef struct {
    byte   type;                /* SUBNET_NODE                          */

    bits   node      :7;        /* Node number                          */
    bits   domain    :1;        /* Domain index                         */

    bits   retry     :4;        /* Retry count                          */
    bits   rpt_timer :4;        /* Retry repeat timer                   */

    bits   tx_timer  :4;        /* Transmit timer index                 */
    bits   rsvd1     :4;

    bits   subnet    :8;        /* Subnet ID                            */
} SendSnode;

/* 48-bit NEURON ID destination address. */

#define NEURON_ID_LEN 6

typedef struct {
    byte   type;                /* NEURON_ID                            */

    bits   rsvd0      :7;
    bits   domain     :1;       /* Domain index                         */

    bits   retry      :4;       /* Retry count                          */
    bits   rpt_timer  :4;       /* Retry repeat timer                   */

    bits   tx_timer   :4;       /* Transmit timer index                 */
    bits   rsvd2      :4;

    bits   subnet     :8;       /* Subnet ID, 0 => pass all routers     */
    byte   nid[ NEURON_ID_LEN ];  /* NEURON ID                          */
} SendNrnid;

/* Broadcast destination address. */

typedef struct {
    byte   type;                /* BROADCAST                            */

    bits   backlog     :6;      /* Backlog                              */
    bits   rsvd0       :1;
    bits   domain      :1;      /* Domain index                         */

    bits   retry       :4;      /* Retry count                          */
    bits   rpt_timer   :4;      /* Retry repeat timer                   */

    bits   tx_timer    :4;      /* Transmit timer index                 */
    bits   rsvd2       :4;

    bits   subnet      :8;      /* Subnet ID, 0 => domain-wide          */
} SendBcast;

/* Address formats for special host addresses.                          */

typedef struct {
    byte   type;                /*  LOCAL                               */
} SendLocal;

typedef struct {
   byte    type;                /* IMPLICIT */
   byte    msg_tag;             /* address table entry number */
} SendImplicit;

/* Union of all destination addresses. */

typedef union {
    SendGroup      gp;
    SendSnode      sn;
    SendBcast      bc;
    SendNrnid      id;
    SendLocal      lc;
    SendImplicit   im;
} SendAddrDtl;

/*
 ****************************************************************************
 * Network Address structures for receiving messages with explicit
 * addressing enabled.
 ****************************************************************************
 */

/* Received subnet/node ID destination address.  Used for unicast messages. */

typedef struct {
    bits       subnet :8;
    bits       node   :7;
    bits              :1;
} RcvSnode;

/* Received 48-bit NEURON ID destination address. */

typedef struct {
    byte   subnet;
    byte   nid[ NEURON_ID_LEN ];
} RcvNrnid;

/* Union of all received destination addresses. */

typedef union {
    byte       gp;                  /* Group ID for multicast destination   */
    RcvSnode   sn;                  /* Subnet/node ID for unicast           */
    RcvNrnid   id;                  /* 48-bit NEURON ID destination address */
    byte       subnet;              /* Subnet ID for broadcast destination  */
                                    /* 0 => domain-wide */
} RcvDestAddr;

/* Source address of received message.  Identifies */
/* network address of node sending the message.    */

typedef struct {
    bits   subnet  :8;
    bits   node    :7;
    bits           :1;
} RcvSrcAddr;

/* Literals for the 'format' field of RcvAddrDtl. */

typedef enum {
    ADDR_RCV_BCAST  = 0,
    ADDR_RCV_GROUP  = 1,
    ADDR_RCV_SNODE  = 2,
    ADDR_RCV_NRNID  = 3
} RcvDstAddrFormat;

/* Address field of incoming message. */

typedef struct {
#ifdef _MSC_VER
    byte    kludge;    /* Microsoft C does not allow odd-length bitfields */
#else
    bits    format      :6;     /* Destination address type             */
                                /* See 'RcvDstAddrFormat'               */
    bits    flex_domain :1;     /* 1 => broadcast to unconfigured node  */
    bits    domain      :1;     /* Domain table index                   */
#endif
    RcvSrcAddr  source;             /* Source address of incoming message   */
    RcvDestAddr dest;               /* Destination address of incoming msg  */
} RcvAddrDtl;

/*
 ****************************************************************************
 * Network Address structures for receiving responses with explicit
 * addressing enabled.
 ****************************************************************************
 */

/* Source address of response message. */

typedef struct {
    bits   subnet   :8;
    bits   node     :7;
    bits   is_snode :1;        /* 0 => Group response,                 */
                               /* 1 => snode response                  */
} RespSrcAddr;

/* Destination of response to unicast request. */

typedef struct {
    bits   subnet   :8;
    bits   node     :7;
    bits            :1;
} RespSnode;

/* Destination of response to multicast request. */

typedef struct {
    bits   subnet   :8;
    bits   node     :7;
    bits            :1;
    bits   group    :8;
    bits   member   :6;
    bits            :2;
} RespGroup;

/* Union of all response destination addresses. */

typedef union {
    RespSnode  sn;
    RespGroup  gp;
} RespDestAddr;

/* Address field of incoming response. */

typedef struct {
#ifdef _MSC_VER
    byte     kludge;     /* Microsoft C does not allow odd-length bitfields */
#else
    bits                 :6;
    bits     flex_domain :1;    /* 1=> Broadcast to unconfigured node   */
    bits     domain      :1;    /* Domain table index                   */
#endif
    RespSrcAddr  source;            /* Source address of incoming response  */
    RespDestAddr dest;              /* Destination address of incoming resp */
} RespAddrDtl;

/* Explicit address field if explicit addressing is enabled. */

typedef union {
    RcvAddrDtl  rcv;
    SendAddrDtl snd;
    RespAddrDtl rsp;
} ExplicitAddr;

/*
 ****************************************************************************
 * Data field structures for explicit messages and network variables.
 ****************************************************************************
 */

/*
 * MAX_NETMSG_DATA specifies the maximum size of the data portion of an
 * application buffer.  MAX_NETVAR_DATA specifies the maximum size of the
 * data portion of a network variable update.  The values specified here
 * are the absolute maximums,based on the LONTALK protocol. Actual limits
 * are based on the buffer sizes defined on the attached NEURON CHIP.
 */

#define MAX_NETMSG_DATA 228
#define MAX_NETVAR_DATA 31

/* Data field for network variables (host selection enabled). */

typedef struct {
    bits   NV_selector_hi :6;
    bits   direction      :1;     /* 1 => output NV, 0 => input NV      */
    bits   must_be_one    :1;     /* Must be set to 1 for NV            */
    bits   NV_selector_lo :8;
    byte       data[ MAX_NETVAR_DATA ]; /* Network variable data            */
} UnprocessedNV;

/* Data field for network variables (network interface selection enabled).  */

typedef struct {
    byte       index;                 /* Index into NV configuration table  */
    byte       rsvd0;
    byte       data[ MAX_NETVAR_DATA ]; /* Network variable data            */
} ProcessedNV;

/* Data field for explicit messages. */

typedef struct {
    byte       code;                  /* Message code                       */
    byte       data[ MAX_NETMSG_DATA ]; /* Message data                     */
} ExplicitMsg;

/* Union of all data fields. */

typedef union {
    UnprocessedNV unv;
    ProcessedNV   pnv;
    ExplicitMsg   exp;
} MsgData;

/*
 ****************************************************************************
 * Message buffer types.
 ****************************************************************************
 */

/* Application buffer when using explicit addressing. */

typedef struct {
    NI_Hdr       ni_hdr;            /* Network interface header             */
    MsgHdr       msg_hdr;           /* Message header                       */
    ExplicitAddr addr;              /* Network address                      */
    MsgData      data;              /* Message data                         */
} ExpAppBuffer;

/* Application buffer when not using explicit addressing. */

typedef struct {
    NI_Hdr       ni_hdr;            /* Network interface header             */
    MsgHdr       msg_hdr;           /* Message header                       */
    MsgData      data;              /* Message data                         */
} ImpAppBuffer;

/*
 ****************************************************************************
 * Network interface error codes.
 ****************************************************************************
 */

typedef enum {
    NI_OK = 0,
    NI_NO_DEVICE,
    NI_DRIVER_NOT_OPEN,
    NI_DRIVER_NOT_INIT,
    NI_DRIVER_NOT_RESET,
    NI_DRIVER_ERROR,
    NI_NO_RESPONSES,
    NI_RESET_FAILS,
    NI_TIMEOUT,
    NI_UPLINK_CMD,
    NI_INTERNAL_ERR,
} NI_Code;

/***************************** Externals *****************************/

NI_Code ni_init( const char * device_name);
                            /* Initialize network interface   */
void ni_close( void );      /* Close network interface */
NI_Code ni_reset( void );   /* Reset network interface        */

    /* Assumes only one network interface is open */

    /* Assume network interface is configured with explicit addressing ON,
       and network variable processing OFF */

NI_Code ni_send_msg_wait(
    ServiceType       service,          /* ACKD, UNACKD_RPT, UNACKD, REQUEST */
    const SendAddrDtl * out_addr,       /* address of outgoing message */
    const MsgData     * out_data,       /* data of outgoing message */
    int               out_length,       /* length of outgoing message */
    bool           priority,         /* outgoing message priority */
    bool           out_auth,         /* outgoing message authenticated */
    ComplType       * completion,       /* MSG_SUCCEEDS or MSG_FAILS */
    int             * num_responses,    /* number of received responses */
    RespAddrDtl     * in_addr,          /* address of first response */
    MsgData         * in_data,          /* data of first response */
    int             * in_length         /* length of first response */
);

NI_Code ni_get_next_response(         /* get subsequent responses here */
    RespAddrDtl     * in_addr,
    MsgData         * in_data,
    int             * in_length
);

NI_Code ni_receive_msg(
    ServiceType     * service,        /* ACKD, UNACKD_RPT, UNACKD, REQUEST */
    RcvAddrDtl      * in_addr,        /* address of incoming msg */
    MsgData         * in_data,        /* data of incoming msg */
    int             * in_length,      /* length of incoming msg */
    bool         * in_auth         /* if incoming was authenticated */
);

NI_Code ni_send_response(         /* send response to last received request */
    MsgData       * out_data,     /* data for outgoing response */
    int             out_length    /* length of outgoing response */
);

NI_Code ni_send_immediate( NI_NoQueueCmd command );
            // send an immediate (no queue) command to network interface */

extern ExpAppBuffer  msg_out;          /* Outgoing message buffer       */
extern ExpAppBuffer  msg_in;           /* Incoming message buffer       */

extern void ni_msg_hdr_init( int msg_size, ServiceType service,
    bool priority, bool local, bool auth, bool implicit,
    byte msg_tag );

void ni_error_display(const char * s, NI_Code ni_error);
                                     /* Display a network interface error*/

void ni_msg_display( ExpAppBuffer *msg_ptr );
void print_array( const void * data, int length, const char * format );

bool handle_error( NI_Code   ni_error,
                      ComplType completion,
                      byte      response_code,
                      const char      * msg_name );
		      
#endif
