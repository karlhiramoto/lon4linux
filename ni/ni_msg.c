/***********

Copyright (c) 2000-2005 Karl Hiramoto

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**********/


//// Include Files 

#include "ldv.h"                    /* Standard network driver functions   */
#include "ni_msg.h"                 /* Network interface data structures   */
#include <stdio.h>                  /* For printf(), etc                   */
#include <string.h>                 /* For strlen(), strncpy()             */
#include <stdlib.h>                 /* For malloc(), free(), exit()        */
#include <time.h>
#include <sys/time.h>

#include <curses.h>
#include <unistd.h>   // for sleep() karl's test

extern bool verbose_flag,network_flag;

/*
 ****************************************************************************
 * Default message buffers.
 * Assume explicit addressing is enabled in network interface.
 ****************************************************************************
 */
 
FILE *LogFilePtr;
char LogFileName[80]="lon_network_log.txt";

ExpAppBuffer  msg_in;               /* Incoming message buffer       */
ExpAppBuffer  msg_out;              /* Outgoing message buffer       */

struct msg_list_struct {
    struct msg_list_struct * next_msg;        /* linked list pointer */
    byte                     length;
    ExplicitAddr             addr;
    ServiceType              service;        /* for pending list messages */
    bool                  priority;       /*           "               */
    bool                  auth;           /*           "               */
    byte                     tag;            /*           "               */
    byte                     data[ 1 ];      /* followed by the real data */
};                 /* linked list of pending messages */

typedef struct msg_list_struct msg_list;
msg_list    * response_list = NULL;       /* List of pending responses */
msg_list    * pending_list  = NULL;       /* List of other incoming msgs */

/* These are from the last request message passed back to the application */
/* The application is expected to call ni_send_response( ) */

bool     last_request_priority;
byte        last_request_tag;

// karl changed to fix timeout on reset problem
const time_t niWAIT_TIME = 15;   /* Wait time for network interface */
//const time_t niWAIT_TIME = 5;   /* Wait time for network interface */
const byte msg_tag_any = 14;
   /* use this tag for explicitly addressed send_msg_wait */

const char  * svc_table[ 4 ]
    = { "Ackd", "Unackd/rpt", "Unackd", "Request" };

void init_response_list( void );    /* prototypes */
void init_pending_list( void );
NI_Code ni_get_msg( time_t wait );
NI_Code ni_put_msg( void );
void ni_ldv_error_display(const char * s, LDVCode ldv_error);
                                     /* Display a network driver error   */
void save_response( byte msg_length );
void save_pending( byte msg_length );

void init_response_list( void );

/*
 ****************************************************************************
 * Network driver error strings.
 ****************************************************************************
 */

static char *ldv_error_strings[] = {
    "No error",
    "Network driver not found",
    "Network interface already open",
    "Network interface not open",
    "Network interface hardware error",
    "Invalid device ID",
    "No messages buffered for read",
    "No message buffers available for write",
    "Network interface being initialized"
};

/*
 ****************************************************************************
 * Network interface error strings.
 ****************************************************************************
 */

static char *ni_error_strings[] = {
    "No error",
    "Device name not specified",
    "Could not open network driver",
    "Could not initialize network driver",
    "Could not reset network driver",
    "Network driver error",
    "No response messages left",
    "Network interface reset failed",
    "No response from network interface, command timed out",
    "Uplink command received from network interface",
    "Internal error detected"
};

/***************************** Local Variables ******************************/

static const char * current_device_name; 
                    /* Currently open network driver name */
static LNI ni_handle = -1;
               /* Network driver handle - initially closed */

/************************************************************************/

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
) {

    bool         local;              // Is it addressed to net intfc?
    bool         implicit;           // Does it use implicit addressing?
    NI_Code         ni_error;
    int             response_count = 0;
    ComplType       cmpl_code;
    byte            incoming_tag;
    byte            outgoing_tag;
    byte            msg_length;

    if( num_responses ) * num_responses = 0;    /* clear count */
    init_response_list( );      /* clear out any responses from previous */

    implicit = ( out_addr->im.type == IMPLICIT );
    outgoing_tag = implicit ? out_addr->im.msg_tag : msg_tag_any;
    local = ( out_addr->lc.type == LOCAL );

    ni_msg_hdr_init( out_length,     /* msg size */
                  service,          /* service type */
                  priority,         /* priority */
                  local,            /* addressed to net intfc */
                  out_auth,         /* authenticated */
                  implicit,         /* explicitly addressed */
                  outgoing_tag  );

    if( !implicit )
        msg_out.addr.snd = * out_addr;      /* destination address */
    memcpy( &msg_out.data.exp, out_data, out_length );    /* outgoing data */
    if( ( ni_error = ni_put_msg( ) ) != NI_OK )        /* send message */
        return( ni_error );

    if( local && service != REQUEST ) {     // local, not req/resp, all done
        if( completion ) * completion = MSG_SUCCEEDS;
        return( NI_OK );
    }
    
     usleep(1000);  // karl test

        for( ;; ) {         /* look for completion events and responses */
            if( ( ni_error = ni_get_msg( niWAIT_TIME ) ) != NI_OK )
                return( ni_error );

            msg_length = msg_in.msg_hdr.exp.length;
            incoming_tag = msg_in.msg_hdr.exp.tag;

            if( msg_in.msg_hdr.exp.response ) {     /* a response */
                if( incoming_tag != outgoing_tag ) {
                    save_pending( msg_length );    /* a response to another tag */
                    continue;           /* keep looking for completion */
                }
                            /* a response to our tag */
                if( !response_count ) {  /* first response, copy it */
                    if( in_addr ) * in_addr = msg_in.addr.rsp;
                    if( in_data ) memcpy( in_data, &msg_in.data, msg_length );
                    if( in_length ) * in_length = msg_length;
                } else save_response( msg_length );
                    /* subsequent response to our tag */

            response_count++;           /* got another one */

            if( ! local ) continue; /* not local, keep looking for completion */

          /* no completion events for local msgs, only one response */

            if( completion ) * completion = MSG_SUCCEEDS;
            if( num_responses ) * num_responses = response_count;
            return( NI_OK );         /* This transaction is done */


//	        sleep(1);  // karl test

        }
        /* here if incoming is not a response */

        cmpl_code = (ComplType) msg_in.msg_hdr.exp.cmpl_code;
        switch( cmpl_code ) {
            case MSG_NOT_COMPL:
                save_pending( msg_length ); /* some other incoming message */
                continue;           /* keep looking for completion */

            case MSG_FAILS:
            case MSG_SUCCEEDS:
                /* found the completion event */
                if( incoming_tag != outgoing_tag ) return( NI_INTERNAL_ERR );
                if( completion ) * completion = cmpl_code;
                if( num_responses ) * num_responses = response_count;
                return( NI_OK );         /* This transaction is done */

            default:
                return( NI_INTERNAL_ERR );   /* Invalid completion code */
        }  /* end switch( cmpl_code ) */
    }   /* end for( ;; ) */
}

/************************************************************************/

NI_Code ni_get_next_response(         /* get subsequent responses here */
    RespAddrDtl     * in_addr,
    MsgData         * in_data,
    int             * in_length
) {
    byte          msg_length;
    msg_list    * next_response;

    if( response_list == NULL ) return( NI_NO_RESPONSES );

    msg_length = response_list->length;
    if( in_addr ) * in_addr = response_list->addr.rsp;
    if( in_data ) memcpy( in_data, response_list->data, msg_length );
    if( in_length ) * in_length = msg_length;

    next_response = response_list->next_msg;
    free( response_list );
    response_list = next_response;
    return( NI_OK );
}
/************************************************************************/


NI_Code ni_receive_msg(
    ServiceType     * service,        /* ACKD, UNACKD_RPT, UNACKD, REQUEST */
    RcvAddrDtl      * in_addr,        /* address of incoming msg */
    MsgData         * in_data,        /* data of incoming msg */
    int             * in_length,      /* length of incoming msg */
    bool         * in_auth
) {
    msg_list        * next_pending;
    NI_Code         ni_error;
    byte            msg_length;

    if( pending_list != NULL ) {    /* got one squirreled away */
        if( service ) * service = pending_list->service;
        if( in_addr ) * in_addr = pending_list->addr.rcv;
        if( in_auth ) * in_auth = pending_list->auth;
        if( in_data ) memcpy( in_data, pending_list->data, pending_list->length );
        if( in_length ) * in_length = pending_list->length;
        if( pending_list->service == REQUEST ) {
            last_request_priority = pending_list->priority;
            last_request_tag = pending_list->tag;
        }           /* save for a response from the application */

        next_pending = pending_list->next_msg;  /* unhook pending message */
        free( pending_list );
        pending_list = next_pending;
        return( NI_OK );
    }

    /* nothing on the pending list, see if there is an incoming message */

    if( ( ni_error = ni_get_msg( 0 ) ) != NI_OK ) return( ni_error );

    if( msg_in.msg_hdr.exp.response ) {
        if (verbose_flag) fprintf(LogFilePtr, "Received a belated response message\n" );
        return( NI_UPLINK_CMD );
    }
    switch( msg_in.msg_hdr.exp.cmpl_code ) {
        case MSG_FAILS:
            if (verbose_flag) fprintf(LogFilePtr, "Received a failure completion event\n" );
            return( NI_UPLINK_CMD );
        case MSG_SUCCEEDS:
            if (verbose_flag) fprintf(LogFilePtr, "Received a success completion event\n" );
            return( NI_UPLINK_CMD );
    }       // fall through for non-completion

    msg_length = msg_in.msg_hdr.exp.length;
    if( service ) * service = (ServiceType) msg_in.msg_hdr.exp.st;
    if( in_length ) * in_length = msg_length;
    if( in_addr ) * in_addr = msg_in.addr.rcv;
    if( in_auth ) * in_auth = msg_in.msg_hdr.exp.auth;
    if( in_data ) memcpy( in_data, &msg_in.data, msg_length );

    if( msg_in.msg_hdr.exp.st == REQUEST ) {
        last_request_priority = msg_in.msg_hdr.exp.priority;
        last_request_tag = msg_in.msg_hdr.exp.tag;
    }           /* save for a response */

    return( NI_OK );
}
/************************************************************************/


NI_Code ni_send_response(         /* send response to last received request */
    MsgData       * out_data,     /* data for outgoing response */
    int             out_length    /* length of outgoing response */
) {
    NI_Code         ni_error;

    msg_out.ni_hdr.q.queue        = last_request_priority ? niNTQ_P : niNTQ;
    msg_out.ni_hdr.q.q_cmd        = niCOMM;
    msg_out.ni_hdr.q.length       = sizeof( ExpMsgHdr )
                                  + sizeof( ExplicitAddr) + out_length;
    msg_out.msg_hdr.exp.tag       = last_request_tag;    /* use same tag */
    msg_out.msg_hdr.exp.auth      = 0;
    msg_out.msg_hdr.exp.response  = 1;       /* make it a response */
    msg_out.msg_hdr.exp.st        = REQUEST;
    msg_out.msg_hdr.exp.pool      = 0;      /* Must be zero             */
    msg_out.msg_hdr.exp.alt_path  = 0;      /* Use default path         */
    msg_out.msg_hdr.exp.addr_mode = 0;      /* IMplicit addressing      */
    msg_out.msg_hdr.exp.cmpl_code = MSG_NOT_COMPL; /* Zero              */
    msg_out.msg_hdr.exp.path      = 0;      /* Use primary path         */
    msg_out.msg_hdr.exp.priority  = last_request_priority;
    msg_out.msg_hdr.exp.length    = out_length;

    memcpy( &msg_out.data.exp, out_data, out_length );    /* outgoing data */
    ni_error = ni_put_msg( );       /* send message */
    return( ni_error );
};

/************************************************************************/

void init_response_list( void ) {   /* initialize list of responses */

    msg_list    * next_response;

    while( response_list != NULL ) {
        next_response = response_list->next_msg;
        free( response_list );
        response_list = next_response;
   }
}
/************************************************************************/

void init_pending_list( void ) {
      /* initialize list of pending messages */

    msg_list    * next_pending;

    while( pending_list != NULL ) {
        next_pending = pending_list->next_msg;
        free( pending_list );
        pending_list = next_pending;
   }
}
/************************************************************************/

void save_response( byte msg_length ) {
     /* come here to save any responses beyond the first for group
        addressed request/response service */

    msg_list    * next_response;

    next_response = ( msg_list * ) malloc( sizeof( msg_list ) + msg_length );
    if( next_response == NULL ) {
        if (verbose_flag) fprintf(LogFilePtr, "Out of memory in save_response\n" );
        exit( 1 );      /* Unfriendly recovery */
    }

    next_response->next_msg = response_list;    /* link chain */
    response_list = next_response;
    next_response->length = msg_length;         /* save response */
    next_response->addr = msg_in.addr;
    memcpy( next_response->data, &msg_in.data, msg_length );
}
/************************************************************************/

void save_pending( byte msg_length ) {
    /* come here to squirrel away an incoming message that is not part
       of the current transaction */
    msg_list    * next_pending;

    next_pending = ( msg_list * )malloc( sizeof( msg_list ) + msg_length );
    if( next_pending == NULL ) {
        if (verbose_flag) fprintf(LogFilePtr, "Out of memory in save_pending\n" );
        exit( 1 );      /* Unfriendly recovery */
    }

    next_pending->next_msg = pending_list;    /* link chain */
    pending_list = next_pending;
    next_pending->length = msg_length;         /* save pending */
    next_pending->addr = msg_in.addr;
    next_pending->service = (ServiceType) msg_in.msg_hdr.exp.st;
    next_pending->priority = msg_in.msg_hdr.exp.priority;
    next_pending->auth = msg_in.msg_hdr.exp.auth;
    memcpy( next_pending->data, &msg_in.data, msg_length );
}

/*
 ****************************************************************************
 * ni_get_msg().  Receive a message in msg_in.  Return when a message
 * is received, or if the wait timer times out
 ****************************************************************************
 */

NI_Code ni_get_msg( time_t wait ) {   /* how long to wait for input */

    LDVCode ldv_error = LDV_OK;
    time_t  deadline;
    timeval TV;

//    printf("In ni_getmsg handle= 0x0%x\n",ni_handle);
    
    if( wait ) deadline = time( NULL ) + wait;

    /* Loop until the network interface provides a message, or timeout */
    for( ;; ) {
        ldv_error = ldv_read( ni_handle, &msg_in, sizeof( msg_in ) );
        /* Check for network interface input */

       // printf("ldv_error=%d \n",ldv_error);

        if( ldv_error == LDV_OK )
	    {
//            fprintf(LogFilePtr, "\n KARL TEST:\n" );
//    	    print_array( &msg_in, msg_in.ni_hdr.q.length+2, "%02X " );
 //      	    print_array( &msg_in, sizeof(msg_in), "%02X " );

	        break;
	     
	    }

        if( ldv_error != LDV_NO_MSG_AVAIL
         && ldv_error != LDV_DEVICE_BUSY ) break;   // non-recoverable

        if( ! wait || time( NULL ) > deadline )
                return( NI_TIMEOUT );       /* no message received */
    }

    if( ldv_error != LDV_OK ) {
        if (verbose_flag) fprintf(LogFilePtr,"LDV_ERROR != LDV_OK  ldv_error = %d \n", ldv_error);
    
        /* Network driver returned error, print it, and reopen driver */
        if (verbose_flag) ni_ldv_error_display( "Network driver error on read", ldv_error );
        if ( ni_init( current_device_name ) != NI_OK )
             if (verbose_flag) fprintf(LogFilePtr, "\nCould not reopen network interface.\n" );
        else
             if (verbose_flag) fprintf(LogFilePtr, "\nReopened network interface.\n" );
        return( NI_DRIVER_ERROR );
    }

    if( msg_in.ni_hdr.q.q_cmd == niCOMM ) {     // incoming message
        if( verbose_flag ) {
	
	   	   if(gettimeofday(&TV,0)==0)
              if (verbose_flag) fprintf(LogFilePtr,"\nReceived a LONTALK Message @ = %sUSeconds = %ld Error = %d\n",ctime(&TV.tv_sec), TV.tv_usec, ldv_error);
           else
              if (verbose_flag) fprintf(LogFilePtr, "\nReceived a LONTALK error = %d\n", ldv_error );

            if( ldv_error == LDV_OK )  ni_msg_display( &msg_in );
        }
        return( NI_OK );
    }
        // something other than an incoming message

    if( msg_in.ni_hdr.noq.cmd == niRESET )
    {
        if (verbose_flag) fprintf(LogFilePtr, "Received uplink local reset\n" );
        ni_send_immediate( niFLUSH_CANCEL );
    }
    else
        if (verbose_flag) fprintf(LogFilePtr, "Received uplink command 0x%x\n", msg_in.ni_hdr.noq.cmd );

    return( NI_UPLINK_CMD );  /* not a message */
}

/*
 ****************************************************************************
 * ni_put_msg().  Send the message assembled in msg_out.
 ****************************************************************************
 */

NI_Code ni_put_msg( void )
{
    LDVCode ldv_error = LDV_OK;
    time_t  deadline;
    timeval TV;

    deadline =  time( NULL ) + niWAIT_TIME;

    // print_array(&msg_out,msg_out.ni_hdr.q.length + sizeof( NI_Hdr ), "%02X "); // karl test

    /* Loop until the network interface accepts the output, or timeout. */
    for( ;; ) {
        ldv_error = ldv_write( ni_handle, &msg_out,
                                msg_out.ni_hdr.q.length + sizeof( NI_Hdr ) );
            /* Attempt to write to the network interface. */

        if( ldv_error == LDV_OK ) break;
        if( ldv_error != LDV_NO_BUF_AVAIL
         && ldv_error != LDV_DEVICE_BUSY ) break;    // non-recoverable
                /* exit loop for fatal err */
        if ( time( NULL ) > deadline )             // see if deadline passed
            /* No response from network interface. */
            return( NI_TIMEOUT );
    }

    if( verbose_flag ) {
	   if(gettimeofday(&TV,0)==0)
           fprintf(LogFilePtr,"Sent a LONTALK Message @ = %sUSeconds = %ld Error = %d\n",ctime(&TV.tv_sec), TV.tv_usec, ldv_error);
       else
           fprintf(LogFilePtr, "\nSent a LONTALK error = %d\n", ldv_error );

        if( ldv_error == LDV_OK )
            ni_msg_display( &msg_out );
    }

    if( ldv_error == LDV_OK ) return( NI_OK );

        /* Network driver returned error, print it, and reopen driver */
    ni_ldv_error_display( "Network driver error on write", ldv_error );
    if ( ni_init( current_device_name ) != NI_OK )
        if (verbose_flag) fprintf(LogFilePtr, "\nCould not reopen network interface.\n" );
    else
        if (verbose_flag) fprintf(LogFilePtr, "\nReopened network interface.\n" );
    return( NI_DRIVER_ERROR );
}

/*
 ****************************************************************************
 * ni_msg_hdr_init().  Initialize an outgoing application buffer
 * using explicit addressing.
 ****************************************************************************
 */

void ni_msg_hdr_init( int msg_size, ServiceType service, bool priority,
        bool local, bool auth, bool implicit, byte msg_tag )
{

    const static NI_Queue queue[ 2 ][ 2 ] =         /* define MIP queue */
        { { niTQ, niTQ_P }, { niNTQ, niNTQ_P } };

    msg_out.ni_hdr.q.queue  = queue[ service == UNACKD ][ priority ];
                /* Transaction queue            */

    msg_out.ni_hdr.q.q_cmd    = local ? niNETMGMT : niCOMM;
             /* Data transfer for network interface or network         */
    msg_out.ni_hdr.q.length   = sizeof( ExpMsgHdr ) + sizeof( ExplicitAddr )
                                + msg_size; /* Header size                  */

    msg_out.msg_hdr.exp.tag       = msg_tag;
    msg_out.msg_hdr.exp.auth      = auth;
    msg_out.msg_hdr.exp.st        = service;
    msg_out.msg_hdr.exp.msg_type  = 0;      /* MIP doesn't process NVs      */
    msg_out.msg_hdr.exp.response  = 0;      /* Not a response message       */
    msg_out.msg_hdr.exp.pool      = 0;      /* Must be zero                 */
    msg_out.msg_hdr.exp.alt_path  = 0;      /* Use default path             */
    msg_out.msg_hdr.exp.addr_mode = ! implicit;   /* Addressing?            */
    msg_out.msg_hdr.exp.cmpl_code = MSG_NOT_COMPL; /* Zero                  */
    msg_out.msg_hdr.exp.path      = 0;      /* Use primary path             */
    msg_out.msg_hdr.exp.priority  = priority;
    msg_out.msg_hdr.exp.length    = msg_size;
                                            /* Message size                 */
}


/*************************/

void print_array( const void * data, int length,const char * format ) {
    int i;
    byte * ptr;

    for( i = 0, ptr = ( byte * ) data; i < length; i++, ptr++ )
        if (verbose_flag) fprintf(LogFilePtr, format, * ptr );
}

/*************************/

void print_address_info(RcvAddrDtl address) {

	fprintf(LogFilePtr,"Address Info:\n");

	fprintf(LogFilePtr,"\n");
	
	if (address.flex_domain)
		fprintf(LogFilePtr,"Flex_domain SET Broadcast to unconfigured node\n");
	else  fprintf(LogFilePtr,"Flex_domain NOT SET Broadcast to configured node\n");

	
	if (address.domain)
 		fprintf(LogFilePtr,"Domain SET   \n");
    else fprintf(LogFilePtr,"Domain NOT SET   \n");
    
    fprintf(LogFilePtr,"Source Address Subnet=  %02X   Source Address Node = %02X \n",address.source.subnet, address.source.node);

    fprintf(LogFilePtr,"Destination Info:\n");

	switch (address.format)
	{
	
		case ADDR_RCV_BCAST:
	    fprintf(LogFilePtr,"Address format = ADDR_RCV_BCAST "); 
        fprintf(LogFilePtr,"Destination Address Subnet=  %02X",address.dest.subnet);
		break;
		
		
		case  ADDR_RCV_GROUP:
	    fprintf(LogFilePtr,"Address format = ADDR_RCV_GROUP "); 
  	    fprintf(LogFilePtr,"GroupID= %02X ",address.dest.gp);
		break;
		
		
		case  ADDR_RCV_SNODE:
	    fprintf(LogFilePtr,"Address format = ADDR_RCV_SNODE "); 
	    fprintf(LogFilePtr,"Subnet= %02X ",address.dest.sn.subnet);
	    fprintf(LogFilePtr,"NodeID= %02X ",address.dest.sn.node);
	    break;
	
	        
		case  ADDR_RCV_NRNID:
        fprintf(LogFilePtr,"Address format = ADDR_RCV_NRNID "); 
        fprintf(LogFilePtr,"Subnet= %02X ",address.dest.id.subnet);
	    fprintf(LogFilePtr,"NeuronID= ");
	    print_array(&address.dest.id.nid,sizeof(address.dest.id.nid),"%02X ");
    	break;
	
	
		default:
        fprintf(LogFilePtr,"Address format = NOT FOUND");  
		
	}	
    fprintf(LogFilePtr,"\n");


}
/*
 ****************************************************************************
 * ni_msg_display().  Display contents of a network interface message.
 ****************************************************************************
 */

void ni_msg_display( ExpAppBuffer * msg_ptr )
{
    int length;

    static const char * cmd_str[ 16 ] = {
        "NULL", "COMM", "NETMGMT", "TIMEOUT", "CRC", "RESET", "FLUSH_C",
        "ONLINE", "OFFLINE", "FLUSH", "FLUSH_IGN", "SLEEP", "(N)ACK",
        "0xD", "0xE", "DRV_CMD" };

    static const char * queue_str[ 16 ] = {
        "0x0", "0x1", "TQ", "TQ_P", "NTQ", "NTQ_P", "RESPONSE", "0x7",
        "INCOMING", "0x9", "0xA", "0xB", "0xC", "0xD", "0xE", "0xF" };

    static const char * compl_str[ 4 ] = {
        "NOT_COMPL", "SUCCEEDS", "FAILS", "cmpl=3" };

//    extern const char * svc_table[ ];       // strings for service type


   fprintf(LogFilePtr,"Ni Header (values in hex):\n");
   print_array(&msg_ptr->ni_hdr, sizeof( NI_Hdr ), "%02X ");
   fprintf(LogFilePtr,"\n");
   
   fprintf(LogFilePtr,"ExpMsgHdr (values in hex):\n");
   print_array(&msg_ptr->msg_hdr, sizeof( MsgHdr ), "%02X ");
   fprintf(LogFilePtr,"\n");


    /* Display headers. */
    fprintf(LogFilePtr,"  Network intfc header: command = ni%s, queue = ni%s, "
              "total length = %d\n", cmd_str[ msg_ptr->ni_hdr.q.q_cmd ],
              queue_str[ msg_ptr->ni_hdr.q.queue ], msg_ptr->ni_hdr.q.length);

    fprintf(LogFilePtr,"  Message header: tag = %d, service = %s, response = %s, "
              "data length = %d\n", msg_ptr->msg_hdr.exp.tag,
              svc_table[ msg_ptr->msg_hdr.exp.st ],
              msg_ptr->msg_hdr.exp.response ? "yes": "no",
              msg_ptr->msg_hdr.exp.length );

    fprintf(LogFilePtr," Explicit address mode = %s, completion code = MSG_%s\n",
              msg_ptr->msg_hdr.exp.addr_mode ? "yes" : "no",
              compl_str[ msg_ptr->msg_hdr.exp.cmpl_code ] );

    /* Display address field */
    fprintf(LogFilePtr, "  Address field (values in hex):\n" );
    print_array( &msg_ptr->addr, sizeof( ExplicitAddr ), "%02X " );
    fprintf(LogFilePtr, "\n" );

    /* Display data field. */
    length = msg_ptr->ni_hdr.q.length
        - sizeof( MsgHdr ) - sizeof( ExplicitAddr );
    if( msg_ptr->msg_hdr.exp.length < length )
        length = msg_ptr->msg_hdr.exp.length;
    if( length ) {
        fprintf(LogFilePtr,"  Data field (values in hex):\n" );
        print_array( &msg_ptr->data, length, "%02X " );
        fprintf(LogFilePtr, "\n" );
    }
}

/*
 ****************************************************************************
 * handle_error - report errors from a LonTalk message. Checks ni_error
 * against NI_OK, completion against MSG_SUCCEEDS, and checks response_code
 * for a successful network management response bit
 * (use NO_CHECK if it's not a network management message, or not a response)
 * Returns false if everything is OK, true if some error occurred.
 ****************************************************************************
 */

bool handle_error( NI_Code   ni_error,
                      ComplType completion,
                      byte      response_code,
                      const char      * msg_name ) {

    if( ni_error != NI_OK ) {
        fprintf(LogFilePtr, "Network error: " );
        ni_error_display( msg_name, ni_error );
        return true;
    }
    if( completion == MSG_FAILS ) {
        fprintf(LogFilePtr, "Message failed completion: %s\n", msg_name );
        return true;
    }
    if( !( response_code & 0x20 ) ) {
        fprintf(LogFilePtr, "Response error: %s\n", msg_name );
        return true;
    }
    return false;
}

/*
 ****************************************************************************
 * ni_init().  Initialize the network interface.  Called on
 * program startup and when network interface errors occur.  Network
 * interface errors can be caused by cycling power on an SLTA with
 * autobaud selected while this program is running.  Reopening the network
 * driver will reinitialize the bit rate on the SLTA interface.
 ****************************************************************************
 */

NI_Code ni_init( const char *device_name )
{
    NI_Code ni_error = NI_OK;
    
    current_device_name = device_name;  // karl added this  07/07/2000   current_device_name was turning up null when ther was an error in ni_put_msg

    LogFilePtr=fopen (LogFileName,"w+");
    fprintf(LogFilePtr,"\n\n----------  STARTING NEW LOG -------- \n\n"); 
    
    if( ni_handle >= 0 )
        /* Network interface has already been opened, close it first. */
        ldv_close( ni_handle );

    //fprintf(LogFilePtr, LAST_LINE, 0, "TEST  ni_init()  device_name=%s \n",device_name);
    if (verbose_flag) fprintf(LogFilePtr,"Initializing PCLTA Interface device = %s \n", device_name);
    /* Open network interface. */
    
//    printf("calling ldv_open\n");
    if( LDV_OK != ldv_open( device_name, &ni_handle ) )
    {
        printf("ERROR ldv_open\n");

        return( NI_DRIVER_NOT_OPEN );
	}
  //  printf("done ldv_open\n");


    /* Empty out any uplink messages */
    //fprintf(LogFilePtr, LAST_LINE, 0, "TEST  ni_init() SUCCESS device open  device_name= %s ;handle=0x0%x\n",device_name,ni_handle);


    do {
        ni_error = ni_get_msg( 1 );
   //     printf("ni_error %d =\n",ni_error);

    } while ( ni_error == NI_OK || ni_error == NI_UPLINK_CMD );

 //   printf("done NI_GET_MSG \n");
    

    //fprintf(LogFilePtr, LAST_LINE, 0, "TEST  ni_init() DONE with emptying buffer. Now calling ni_reset() device_name= %s \n",device_name);

    /* Reset the network interface. */
    ni_error = ni_reset();
    if (ni_error != NI_OK) {
        /* Network driver returned error, print it and return. */
        ni_error_display("Network interface error on reset", ni_error);
        return(NI_DRIVER_NOT_RESET);
    } else
        current_device_name = device_name;

    return(ni_error);
}

/*
 ****************************************************************************
 * ni_close().  Close network interface.
 ****************************************************************************
 */

void ni_close( void ) {
    ldv_close( ni_handle );
    fclose(LogFilePtr);
}

/*
 ****************************************************************************
 * ni_reset().  Reset network interface.  Wait for response.  Return error
 * code.
 ****************************************************************************
 */

NI_Code ni_reset(void)
{
    LDVCode ldv_error = LDV_OK;
    NI_Code ni_error  = NI_OK;
    time_t  deadline;

    init_pending_list( );       /* throw away any unprocessed messages */

    if (verbose_flag)
        if (verbose_flag) fprintf(LogFilePtr, "\nReset network interface.\n" );
	
//	sleep(1);  // Karl's test 17/07/00
//	fprintf(LogFilePtr,"TEST:  in ni_reset()  sending_immediate niRESET \n");
    ni_error = ni_send_immediate( niRESET );
    if( ni_error != NI_OK )
        return ni_error;

//	sleep(1); // Karl's test 17/07/00
    deadline = time( NULL ) + niWAIT_TIME;

//	fprintf(LogFilePtr,"TEST:  in ni_reset()  waiting for response from niRESET \n");

    do {
        /* Loop until an input is received from the network interface. */
        do {
            /* Check for timeout. */
            if ( time( NULL ) > deadline )
                /* No response from network interface. */
                return(NI_TIMEOUT);

            /* Check for network interface input. */
            ldv_error = ldv_read( ni_handle, &msg_in, sizeof( msg_in ) );
	      //  sleep(1);
        } while (ldv_error == LDV_NO_MSG_AVAIL
                 || ldv_error == LDV_DEVICE_BUSY);

        if (verbose_flag) {
            fprintf(LogFilePtr,"\nReceived a LONTALK message, error = %d\n", ldv_error);
            if (ldv_error == LDV_OK)
                ni_msg_display(&msg_in);
        }

        if (ldv_error != LDV_OK) {
            /* Network driver returned error, display it, and return error. */
            ni_ldv_error_display("IN ni_reset(). Network driver error on reset", ldv_error);
            return(NI_DRIVER_ERROR);
        }

        if (msg_in.msg_hdr.exp.cmpl_code == MSG_FAILS)
            /* Failure completion code. */
            return(NI_RESET_FAILS);
    } while( msg_in.ni_hdr.noq.cmd != niRESET );

    return ni_send_immediate( niFLUSH_CANCEL );  /* response */
}
/*
 ****************************************************************************
 * ni_send_immediate( ).  Send an immediate command to the network interface
 ****************************************************************************
 */

NI_Code ni_send_immediate( NI_NoQueueCmd command ) {
    msg_out.ni_hdr.noq.cmd = command;
    msg_out.ni_hdr.noq.length = 0;
    return ni_put_msg( );
}

/*
 ****************************************************************************
 * ni_error_display().  Display a network interface error message.  Display
 * a user specified string first.
 ****************************************************************************
 */

void ni_error_display(const char *s, NI_Code ni_error)
{
    if ((strlen(s) + strlen(ni_error_strings[ni_error])) < 78)
        fprintf(LogFilePtr,"%s: %s", s, ni_error_strings[ni_error]);
    else
    {
        /* Message too long for one line, split on two lines. */
        fprintf(LogFilePtr,"%s: ", s );
        fprintf(LogFilePtr,"%s", ni_error_strings[ni_error]);
	}
}

/*
 ****************************************************************************
 * ni_ldv_error_display().  Display a network driver error message.  Display
 * a user pecified string first.
 ****************************************************************************
 */

void ni_ldv_error_display(const char *s, LDVCode ldv_error)
{
        fprintf(LogFilePtr,"%s: %s", s, ldv_error_strings[ldv_error]);
}
