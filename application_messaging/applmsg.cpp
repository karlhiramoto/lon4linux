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


#ifndef _APPLMSG_C
#define _APPLMSG_C	1


#define MAX_ERROR_RETRY_COUNT 3


#include "applmsg.h"
 

extern void swab(const void *from, void *to, size_t n);

/*************************************************************
 *
 *          Declare network variable configuration table
 *
 *************************************************************/

#define NUM_NVS 8
            /* Number of Network Variable table entries on this node */

const int num_nvs = NUM_NVS;          // for external references
nv_struct nv_config_table[ NUM_NVS ]  // configuration table
    = {
/*        selhi  dir     prio  sello  addridx   auth  svc   trnarnd    */
        { 0x3F, NV_OUT, false, 0xFF, NULL_IDX, false, ACKD, false },
        { 0x3F, NV_OUT, false, 0xFE, NULL_IDX, false, ACKD, false },
        { 0x3F, NV_OUT, false, 0xFD, NULL_IDX, false, ACKD, false },
        { 0x3F, NV_OUT, false, 0xFC, NULL_IDX, false, ACKD, false },
        { 0x3F, NV_IN , false, 0xFB, NULL_IDX, false, ACKD, false },
        { 0x3F, NV_IN , false, 0xFA, NULL_IDX, false, ACKD, false },
        { 0x3F, NV_IN , false, 0xF9, NULL_IDX, false, ACKD, false },
        { 0x3F, NV_IN , false, 0xF8, NULL_IDX, false, ACKD, false }
    };                                     // saved to disk


/*************************************************************
 *
 *          Declare SNVT self identification storage
 *
 *************************************************************/

/* There are eight Network Variables and three message tags defined
   These are the equivalent Neuron C declarations:

    #pragma enable_sd_nv_names
    #pragma set_node_sd_string "Sample host application program"

    network output sd_string( "ASCII string output NV" )
            SNVT_str_asc  NV_string_out;
    network output sd_string( "Discrete output NV" )
            SNVT_lev_disc NV_disc_out;
    network output sd_string( "Continuous output NV" )
            SNVT_lev_cont NV_cont_out;
    network output sd_string( "Floating count output NV" )
            SNVT_count_f  NV_float_out;

    network input sd_string( "ASCII string input NV" )
            SNVT_str_asc   NV_string_in;
    network input sd_string( "Discrete input NV" )
            SNVT_lev_disc  NV_disc_in;
    network input sd_string( "Continuous input NV" )
            SNVT_lev_cont  NV_cont_in;
    network input sd_string( "Floating count input NV" )
            SNVT_count_f   NV_float_in;

    msg_tag tag_1;
    msg_tag tag_2;
    msg_tag tag_3;

*/

// Network variable names

#define SO_name    "NV_string_out"
#define DO_name    "NV_disc_out"
#define CO_name    "NV_cont_out"
#define FO_name    "NV_float_out"
#define SI_name    "NV_string_in"
#define DI_name    "NV_disc_in"
#define CI_name    "NV_cont_in"
#define FI_name    "NV_float_in"

// Self-documentation strings

#define SO_doc    "ASCII string output NV"
#define DO_doc    "Discrete output NV"
#define CO_doc    "Continuous output NV"
#define FO_doc    "Floating count output NV"
#define SI_doc    "ASCII string input NV"
#define DI_doc    "Discrete input NV"
#define CI_doc    "Continuous input NV"
#define FI_doc    "Floating count input NV"

#define Node_doc  "Sample host application program"

const struct {
    snvt_struct_v0      header;            // Version 0 SNVT_struct
    snvt_desc_struct    string_out_desc;
    snvt_desc_struct    disc_out_desc;
    snvt_desc_struct    cont_out_desc;
    snvt_desc_struct    float_out_desc;
    snvt_desc_struct    string_in_desc;
    snvt_desc_struct    disc_in_desc;
    snvt_desc_struct    cont_in_desc;
    snvt_desc_struct    float_in_desc;

    char                node_doc_string[ sizeof( Node_doc ) ];

    snvt_ext_rec_mask   string_out_mask;
    char                string_out_name[ sizeof( SO_name ) ];
    char                string_out_doc[ sizeof( SO_doc ) ];
    snvt_ext_rec_mask   disc_out_mask;
    char                disc_out_name[ sizeof( DO_name ) ];
    char                disc_out_doc[ sizeof( DO_doc ) ];
    snvt_ext_rec_mask   cont_out_mask;
    char                cont_out_name[ sizeof( CO_name ) ];
    char                cont_out_doc[ sizeof( CO_doc ) ];
    snvt_ext_rec_mask   float_out_mask;
    char                float_out_name[ sizeof( FO_name ) ];
    char                float_out_doc[ sizeof( FO_doc ) ];
    snvt_ext_rec_mask   string_in_mask;
    char                string_in_name[ sizeof( SI_name ) ];
    char                string_in_doc[ sizeof( SI_doc ) ];
    snvt_ext_rec_mask   disc_in_mask;
    char                disc_in_name[ sizeof( DI_name ) ];
    char                disc_in_doc[ sizeof( DI_doc ) ];
    snvt_ext_rec_mask   cont_in_mask;
    char                cont_in_name[ sizeof( CI_name ) ];
    char                cont_in_doc[ sizeof( CI_doc ) ];
    snvt_ext_rec_mask   float_in_mask;
    char                float_in_name[ sizeof( FI_name ) ];
    char                float_in_doc[ sizeof( FI_doc ) ];

} SNVT_info = {
                                                  // SNVT structure
    { sizeof( SNVT_info ) >> 8,    // length_hi
      sizeof( SNVT_info ) & 0xFF,  // length_lo
      NUM_NVS,                  // num_netvars
      0,                        // version
      3                         // mtag_count
    },

    // These SNVT descriptors have ext_rec, nv_service_type_config,
    // nv_priority_config and nv_auth_config set.  In addition, the inputs
    // have nv_polled set, since the application polls them.

    { 0, 1, 1, 1, 0, 0, 0, 1, SNVT_str_asc },    // SNVT descriptors
    { 0, 1, 1, 1, 0, 0, 0, 1, SNVT_lev_disc },
    { 0, 1, 1, 1, 0, 0, 0, 1, SNVT_lev_cont },
    { 0, 1, 1, 1, 0, 0, 0, 1, SNVT_count_f  },
    { 0, 1, 1, 1, 0, 1, 0, 1, SNVT_str_asc  },
    { 0, 1, 1, 1, 0, 1, 0, 1, SNVT_lev_disc },
    { 0, 1, 1, 1, 0, 1, 0, 1, SNVT_lev_cont },
    { 0, 1, 1, 1, 0, 1, 0, 1, SNVT_count_f  },

    Node_doc,                       // node self-documentation string

    // These extension record masks have nm and sd set
    // For Microsoft C, the initializers are hex bytes, since odd-length
    // bitfields are not allowed.

#ifdef _MSC_VER
    { 0x30 },
#else
    { 0, 0, 1, 1, 0, 0 },           // extension record mask
#endif
    SO_name,                        // name
    SO_doc,                         // self-doc

#ifdef _MSC_VER
    { 0x30 },
#else
    { 0, 0, 1, 1, 0, 0 },           // extension record mask
#endif
    DO_name,                        // name
    DO_doc,                         // self-doc

#ifdef _MSC_VER
    { 0x30 },
#else
    { 0, 0, 1, 1, 0, 0 },           // extension record mask
#endif
    CO_name,                        // name
    CO_doc,                         // self-doc

#ifdef _MSC_VER
    { 0x30 },
#else
    { 0, 0, 1, 1, 0, 0 },           // extension record mask
#endif
    FO_name,                        // name
    FO_doc,                         // self-doc

#ifdef _MSC_VER
    { 0x30 },
#else
    { 0, 0, 1, 1, 0, 0 },           // extension record mask
#endif
    SI_name,                        // name
    SI_doc,                         // self-doc

#ifdef _MSC_VER
    { 0x30 },
#else
    { 0, 0, 1, 1, 0, 0 },           // extension record mask
#endif
    DI_name,                        // name
    DI_doc,                         // self-doc

#ifdef _MSC_VER
    { 0x30 },
#else
    { 0, 0, 1, 1, 0, 0 },           // extension record mask
#endif
    CI_name,                        // name
    CI_doc,                         // self-doc

#ifdef _MSC_VER
    { 0x30 },
#else
    { 0, 0, 1, 1, 0, 0 },           // extension record mask
#endif
    FI_name,                        // name
    FI_doc                          // self-doc
};

/*************************************************************
 *
 *          Declare network variable storage
 *
 *************************************************************/


// prototypes for I/O functions

void print_asc( byte * );     // print routines for SNVT values
void print_disc( byte * );
void print_cont( byte * );
void print_float( byte * );
void read_asc( byte * );      // read SNVT values from keyboard
void read_disc( byte * );
void read_cont( byte * );
void read_float( byte * );

network_variable nv_value_table[ NUM_NVS ] =  {  // RAM storage for NVs
     { 31,  NV_OUT, SNVT_info.string_out_name, print_asc,  read_asc   },
     {  1,  NV_OUT, SNVT_info.disc_out_name,   print_disc, read_disc  },
     {  1,  NV_OUT, SNVT_info.cont_out_name,   print_cont, read_cont  },
     {  4,  NV_OUT, SNVT_info.float_out_name,  print_float,read_float },
     { 31,  NV_IN,  SNVT_info.string_in_name,  print_asc,  read_asc   },
     {  1,  NV_IN,  SNVT_info.disc_in_name,    print_disc, read_disc  },
     {  1,  NV_IN,  SNVT_info.cont_in_name,    print_cont, read_cont  },
     {  4,  NV_IN,  SNVT_info.float_in_name,   print_float,read_float }
};

        // Statistics accumulators for received traffic

unsigned num_messages       = 0;
unsigned num_polls          = 0;
unsigned num_updates        = 0;
unsigned long data_length   = 0;



bool report_flag  = true;     // report incoming application msgs
bool verbose_flag = true;    // display all network interface msgs
bool online_flag  = true;     // This node is on-line

//bool report_flag  = true;     // report incoming application msgs
//bool verbose_flag = false;    // display all network interface msgs
//bool online_flag  = true;     // This node is on-line


RcvAddrDtl     last_rcv_addr;    // for debugging
bool        last_rcv_auth;

       // Utility function to get an nv_index from a message
       // 2nd arg is pointer to pointer to next byte after nv_index

word handle_nv_index( MsgData * in_data, void ** next_byte_ptr );

extern bool                      // define forward references
    handle_update_nv_cnfg( MsgData *, ServiceType ),
    handle_query_nv_cnfg( MsgData *, ServiceType ),
    handle_set_mode( MsgData * ),
    handle_query_SNVT( MsgData *, ServiceType ),
    handle_NV_fetch( MsgData *, ServiceType ),
    handle_netvar_msg( MsgData *, ServiceType, int in_length, bool auth ),
    handle_explicit_msg( MsgData *, ServiceType, int in_length,
            bool in_auth );
	    
	    
unsigned int NumNetworkNodes=0;

long long convert_byte_array_to_int(const void *data,int length);


typedef enum 
{	
	DATA = 0,
	END_OF_FILE = 1,
	EXTENDED_ADDRESS = 2,
	START_SEGMENT_ADDRESS = 3,
	EXTENDED_LINEAR_ADDRESS = 4,
	START_LINEAR_ADDRESS = 5,
} hex_record_type;


typedef struct hex_file_line_type
{
 	char record_marker;  // always a  :
	byte record_length;
	word address;
	hex_record_type record_type; 
	byte data[32];
	byte checksum;

} hex_file_line_type;



/*************************************************************
 *
 *          Handle incoming messages to this node -
 *          called from main event loop when message is received
 *
 *************************************************************/

bool process_msg( ServiceType    service,
                     RcvAddrDtl   * address,
                     MsgData      * in_data,
                     int            in_length,
                     bool        in_auth ) {

          /* handle incoming messages addressed to this node */
          /* return true if prompt should be redisplayed */

    last_rcv_addr = * address;        // save for debug purposes

    switch( in_data->exp.code ) {        // dispatch on message code

    case NM_update_nv_cnfg:
        return handle_update_nv_cnfg( in_data, service );

    case NM_query_nv_cnfg:
        return handle_query_nv_cnfg( in_data, service );

    case NM_set_node_mode:
        return handle_set_mode( in_data );

    case NM_query_SNVT:
        return handle_query_SNVT( in_data, service );

    case NM_wink:
        if( report_flag )fprintf(lon_msg_log, "Received Wink network mgmt msg\n" );
        fprintf(lon_msg_log, "\a" );      // Ding!
        return report_flag;

    case NM_NV_fetch:
        return handle_NV_fetch( in_data, service );

    default:    /* handle all other messages here */

        if( in_data->unv.must_be_one )         // This is a net var
             return handle_netvar_msg( in_data, service, in_length, in_auth );
                                               // This is an explicit msg
        else return handle_explicit_msg( in_data, service, in_length, in_auth );

    }               // end switch
}

/*************************************************************
 *
 *          Handle incoming message to update NV config table
 *
 *************************************************************/

bool handle_update_nv_cnfg( MsgData * in_data, ServiceType service ) {

    nv_struct                   * nv_cnfg_ptr;
    word                          nv_index;
    MsgData                       response;
    NI_Code                       ni_error;

    ni_error= NI_OK;
    
    // When an update_nv_cnfg message is received, the new NV config
    // is copied to the NV config table of this node

    nv_index =  handle_nv_index( in_data, (void**)&nv_cnfg_ptr );

//    nv_index =  handle_nv_index( in_data, ( void * )& nv_cnfg_ptr );
                                // get requested index from msg

    if( nv_index >= NUM_NVS ) {   // too big an index
        response.exp.code = NM_update_nv_cnfg_fail;
    } else {
        response.exp.code = NM_update_nv_cnfg_succ;
        nv_config_table[ nv_index ] = * nv_cnfg_ptr;
            // save nv config table entry
    }

    if( service == REQUEST ) ni_error = ni_send_response( &response, 1 );
    if( report_flag ) fprintf(lon_msg_log,
        "Received Update NV config network mgmt msg for index %d\n",
            nv_index );
    if( service == REQUEST )
        ( void ) handle_error( ni_error, MSG_SUCCEEDS, NO_CHECK,
        "sending response" );
    return report_flag;
}

/*************************************************************
 *
 *          Handle incoming message to query NV config table
 *
 *************************************************************/

bool handle_query_nv_cnfg( MsgData * in_data, ServiceType service ) {

    word     nv_index;
    NM_query_nv_cnfg_response   nv_cnfg_rsp_msg;
    int      out_length;
    NI_Code  ni_error;

    // When a query_nv_cnfg message is received, the corresponding
    // entry in the NV config table of this node is returned in a
    // response

    if( service != REQUEST ) return false;
    nv_index = handle_nv_index( in_data, NULL );
                           // get requested nv_index from message

    if( nv_index >= NUM_NVS ) {   // too big an index
        nv_cnfg_rsp_msg.code = NM_query_nv_cnfg_fail;
        out_length = 1;
    } else {
        nv_cnfg_rsp_msg.code = NM_query_nv_cnfg_succ;
        nv_cnfg_rsp_msg.nv_cnfg = nv_config_table[ nv_index ];
                   // get nv config table entry
        out_length = sizeof( NM_query_nv_cnfg_response );
    }
    ni_error = ni_send_response( ( MsgData * ) &nv_cnfg_rsp_msg, out_length );
    if( report_flag ) fprintf(lon_msg_log,
        "Received Query NV config network mgmt msg for index %d\n",
            nv_index );
    ( void ) handle_error( ni_error, MSG_SUCCEEDS, NO_CHECK,
        "sending response" );
    return report_flag;
}

/*************************************************************
 *
 *      Handle incoming message to set the mode of this node
 *
 *************************************************************/

bool handle_set_mode( MsgData * in_data ) {

//    extern ExpAppBuffer msg_out;   /* Outgoing message buffer   */
    NI_Code             ni_error;

    // When a set_node_mode (on/off) line is received, the corresponding
    // command is sent down to the network interface

    online_flag = ( in_data->exp.data[ 0 ] == APPL_ONLINE );
    ni_error = ni_send_immediate( online_flag ? niONLINE : niOFFLINE );

    if( report_flag ) fprintf(lon_msg_log, "Received Set Node Mode network mgmt msg" );
    if( handle_error( ni_error, MSG_SUCCEEDS, NO_CHECK,
        "downlink mode change" ) ) return true;

    if( report_flag )fprintf(lon_msg_log, " - set host node to %sline\n",
         online_flag ? "on" : "off" );
    return report_flag;
}

/*************************************************************
 *
 *      Handle incoming message to query this node's SNVT info
 *
 *************************************************************/

bool handle_query_SNVT( MsgData * in_data, ServiceType service ) {

    unsigned int         out_length;
    MsgData     response;
    NI_Code     ni_error;
    NM_query_SNVT_request * query_SNVT_req_msg;
    union {
        word        offset;
        struct {
            byte    lo;
            byte    hi;
        } bytes;
    } o;

    if( service != REQUEST ) return false;
    query_SNVT_req_msg = ( NM_query_SNVT_request * ) in_data;

        // get the offset in Intel byte-ordering

    o.bytes.hi = query_SNVT_req_msg->offset_hi;
    o.bytes.lo = query_SNVT_req_msg->offset_lo;
    out_length = query_SNVT_req_msg->count; // number of bytes requested

    if( o.offset + out_length > sizeof( SNVT_info ) ) {
         response.exp.code = NM_query_SNVT_fail;
         out_length = 1;            // invalid request
    } else {
        response.exp.code = NM_query_SNVT_succ;
        memcpy( response.exp.data, ( byte * ) & SNVT_info + o.offset,
            out_length );   // copy data from the SNVT table to the response
        out_length++;       // for the code
    }

    ni_error = ni_send_response( &response, out_length );

    if( report_flag )fprintf(lon_msg_log, "Received Query SNVT network mgmt msg\n" );
    ( void ) handle_error( ni_error, MSG_SUCCEEDS, NO_CHECK,
        "sending response" );
    return report_flag;
}

/*************************************************************
 *
 *      Handle incoming NM message to fetch the value of an NV
 *
 *************************************************************/

bool handle_NV_fetch( MsgData * in_data, ServiceType service ) {

    word    nv_index;
    MsgData response;
    int     out_length;
    network_variable * nv_value_ptr;
    NI_Code ni_error;
    
    nv_value_ptr=NULL;
    
    if( service != REQUEST ) return false;

     // When an NV_fetch message is received, value is fetched
    nv_index = handle_nv_index( in_data, NULL );
                        // get requested NV index from msg
    if( nv_index >= NUM_NVS ) {
        response.exp.code = NM_NV_fetch_fail;
        out_length = 1;
    } else {          // construct a response
        nv_value_ptr = & nv_value_table[ nv_index ];
        response.exp.code = NM_NV_fetch_succ;
        response.exp.data[ 0 ] = nv_index;
        memcpy( &response.exp.data[ 1 ], nv_value_ptr->data,
                nv_value_ptr->size );
        out_length = nv_value_ptr->size + 2;   // add code and index
    }

    ni_error = ni_send_response( &response, out_length );
    if( report_flag ) fprintf(lon_msg_log,
        "Received NV fetch network mgmt msg for %s\n", nv_value_ptr->name );
    ( void ) handle_error( ni_error, MSG_SUCCEEDS, NO_CHECK,
         "sending response" );
    return report_flag;
}

/*************************************************************
 *
 *      match_nv_cnfg - find an entry in NV config table
 *      returns index of entry, or -1 if not found
 *
 *************************************************************/

static int match_nv_cnfg( UnprocessedNV * in_nv ) {

    nv_struct   * nv_cnfg_ptr;
    int         nv_index;
    nv_cnfg_ptr = nv_config_table;       // point to NV config table

    for( nv_index = 0; nv_index < NUM_NVS; nv_index++ ) {
        if( in_nv->NV_selector_lo == nv_cnfg_ptr->selector_lo
        &&  in_nv->NV_selector_hi == nv_cnfg_ptr->selector_hi
        &&  in_nv->direction == nv_cnfg_ptr->direction )
            return nv_index;
        nv_cnfg_ptr++;               // next config table entry
    }
    return -1;          // not found - return error
}

/*************************************************************
 *
 *          Handle incoming NV messages to this node
 *
 *************************************************************/

bool handle_netvar_msg( MsgData    * in_data,
                           ServiceType  service,
                           int          in_length,
                           bool      in_auth ) {

    nv_struct         * nv_cnfg_ptr;
    network_variable  * nv_value_ptr;
    int                 nv_index;
    int                 out_length;
    MsgData             response;
    NI_Code             ni_error;
    bool             auth_OK=true;

    nv_value_ptr=NULL; // karl added to remove warning
    
    // Find this NV in the NV config table
    nv_index = match_nv_cnfg( &in_data->unv );

    if( nv_index >= 0 ) {       // found the NV
        nv_cnfg_ptr = &nv_config_table[ nv_index ];
                        // point to NV config table
        nv_value_ptr = & nv_value_table[ nv_index ];
                        // point to NV data table
        auth_OK = in_auth || !nv_cnfg_ptr->auth;
        // Message is authentic if it was authenticated by the Neuron or
        // authentication is not configured for this NV
    }

    if( service == REQUEST ) {    // it's an NV poll
        response.unv.NV_selector_hi = in_data->unv.NV_selector_hi;
        response.unv.direction = ~ in_data->unv.direction;
            // reverse the direction bit to send it back
        response.unv.NV_selector_lo = in_data->unv.NV_selector_lo;
        response.unv.must_be_one = 1;

        if( nv_index >= 0 && auth_OK && online_flag ) {
                       // there was a match
            memcpy( response.unv.data, nv_value_ptr->data, nv_value_ptr->size );
                    // fetch the value
            out_length = nv_value_ptr->size + 2;      // including selector
            num_polls++;
        } else out_length = 2;       // no match or off-line, respond anyway

        ni_error = ni_send_response( &response, out_length );
                    // add the selector size
        if( report_flag && nv_index >= 0 ) fprintf(lon_msg_log, "Received poll for %s\n",
                nv_value_ptr->name );

        ( void ) handle_error( ni_error, MSG_SUCCEEDS, NO_CHECK,
            "sending response" );
        return report_flag;
    }

    // Come here if it's not a poll
    in_length -= 2;        // subtract out the selector size

    if( nv_index < 0                      // NV not found
      || !auth_OK                         // failed authentication
      || in_length != nv_value_ptr->size )  // size mismatch
      return false;                       // done if not a valid update

    memcpy( nv_value_ptr->data, in_data->unv.data, in_length );
           // update NV
    num_updates++;
    if( report_flag )
        fprintf(lon_msg_log, "Updated %s with : ", nv_value_ptr->name );
         nv_value_ptr->print_func( nv_value_ptr->data );

    return report_flag;
}

/*************************************************************
 *
 *    Internal routine to update the value of a network variable
 *
 *************************************************************/

static void update_nv_value( int nv_index,
                      byte * data,
                      int    size ) {

    network_variable  * nv_value_ptr;

    nv_value_ptr = & nv_value_table[ nv_index ];  // point to NV data table
    if( size != nv_value_ptr->size ) return;
            // Reject update if wrong size
    memcpy( nv_value_ptr->data, data, size );   // update the NV value

    num_updates++;
    fprintf(lon_msg_log, "Updated %s with : ", nv_value_ptr->name );
    nv_value_ptr->print_func( nv_value_ptr->data );
}

/*************************************************************
 *
 *    Handle incoming NV poll responses
 *
 *************************************************************/

void handle_netvar_poll_response(
       UnprocessedNV * in_nv,
       int          in_length,
       int          nv_index ) {

    nv_struct         * nv_cnfg_ptr;

    if( report_flag )fprintf(lon_msg_log, "Received network variable poll response\n" );
    // Match on direction and selector bits ( not the first bit )

    nv_cnfg_ptr = &nv_config_table[ nv_index ];   // point to NV config table

    if( in_nv->NV_selector_lo != nv_cnfg_ptr->selector_lo ) return;
    if( in_nv->NV_selector_hi != nv_cnfg_ptr->selector_hi ) return;
    if( in_nv->direction != nv_cnfg_ptr->direction ) return;

    update_nv_value( nv_index, in_nv->data, in_length - 2 );
        // data size is msg size less selector size
}

/*************************************************************
 *
 *          Handle incoming explicit messages to this node
 *
 *************************************************************/

bool handle_explicit_msg( MsgData      * in_data,
                             ServiceType    service,
                             int            in_length,
                             bool        in_auth ) {

    extern  const char * svc_table[ ];       // strings for service type
    byte    response;
    NI_Code ni_error;

    num_messages++;
    data_length += ( in_length - 1 );     // not counting code

        // if node is off-line and a request message is received
    if( service == REQUEST && !online_flag ) {
        response = ( in_data->exp.code >= NA_foreign_msg ) ?
             NA_foreign_offline : NA_appl_offline;

        // return off-line response
        ni_error = ni_send_response( ( MsgData * ) &response, 1 );
        ( void ) handle_error( ni_error, MSG_SUCCEEDS, NO_CHECK,
            "sending response" );
    }
    if( report_flag ) {

        fprintf(lon_msg_log, "Received explicit application message\n" );
        fprintf(lon_msg_log, "Service type %s\n", svc_table[ service ] );
        fprintf(lon_msg_log, "Message code = 0x%x\n", in_data->exp.code );
        fprintf(lon_msg_log, "Authentication %s\n", in_auth ? "on" : "off" );
        fprintf(lon_msg_log, "Length = %d\n", in_length );
    }
    return report_flag;
}

/*************************************************************
 *
 *      Utility function to return a possibly escaped
 *      NV index from a message.  Used by query/update NV config
 *      and NV fetch network management messages
 *
 *************************************************************/
//extern void swab(const void *from, void *to, size_t n);


word handle_nv_index( MsgData * in_data, void ** next_byte_ptr ) {
    word nv_index;

    byte * msg_data_ptr = in_data->exp.data;
            // point to first data byte of message

    nv_index = * msg_data_ptr ++;  // get first byte of message

    if( nv_index == 0xFF ) {        // this is an escape sequence
                        // convert Neuron long to Intel word
    // swab( ( char * )msg_data_ptr++, ( char * )&nv_index, 2 );
    memcpy((char *) &nv_index +1 ,msg_data_ptr,1);
    memcpy((char *) &nv_index, (char*) msg_data_ptr +1,1);

        msg_data_ptr++;            // point past the long
    }
    if( next_byte_ptr ) * next_byte_ptr = msg_data_ptr;
            // return pointer to byte after the index
    return nv_index;
}


/////////////////////////////////////////////////////////////////////////////////////////////
//  Function:    print_neuron_status_struct
//
//  Author: Karl Hiramoto
//  
//  Date:  16 Aug 2000
//
//  Description:  Used to print the data returned from query_node()  
//
//
//  Parameters:  The status struct returned from query status command
//
//  Return Value: void
//
//  Modification Log:
//
//
///////////////////////////////////////////////////////////////////////////////////////////
void print_neuron_status_struct(status_struct *SS)
{
	fprintf(lon_msg_log,"xmit_errors = %04d         transaction_timeouts = %04d \n",SS->xmit_errors,SS->transaction_timeouts);
	fprintf(lon_msg_log,"rcv_transaction_full=%04d  lost_msgs=%d \n",SS->rcv_transaction_full,SS->lost_msgs);
	fprintf(lon_msg_log,"missed_msgs = %04d         reset_cause = %d \n",SS->missed_msgs,SS->reset_cause);
	fprintf(lon_msg_log,"node_state = 0x%02x        version_number = %d \n",SS->node_state,SS->version_number);
	fprintf(lon_msg_log,"error_log = %04d           model_number = %d  \n",SS->error_log, SS->model_number);
    return;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//  Function: leave_domain
//
//  Author: Karl Hiramoto
//  
//  Date:  16 Aug 2000
//
//  Description:  Command used to issue the leave domain command
//
//
//  Parameters: the address to send  the command to
//
//  Return Value: bool.  TRUE on success.  FALSE on ERROR
//
//  Modification Log:
//
//
///////////////////////////////////////////////////////////////////////////////////////////
bool leave_domain(SendAddrDtl  *send_addr)
{

	NI_Code      status = NI_OK;
	//SendAddrDtl outAddr;
	MsgData     outData;
	MsgData     response_data;
	int         response_length;
	ComplType   completion;
	int num_responses=0;
	RespAddrDtl response_addr;
	int	retry_count=0;


	outData.exp.code = 0x64; // leave domain
    outData.exp.data[0]=0;  // domain index
    outData.exp.data[1]=0;  

	retry_leave_domain:
	
	response_data.exp.code= 0; // init


   /* Ship out the message and wait for the response: */
   status = ni_send_msg_wait(REQUEST,
   				  send_addr,  // send to address
    			 &outData,  // send data
				  2,        // data length
				  false,	// priority
				  false, // send_auth
                  &completion, // completion code
		          &num_responses,  // number of responses
			      &response_addr,	// resp_addr
			      &response_data,  // resp_data
			      &response_length);  // resp length

   if (status == NI_OK && completion != MSG_SUCCEEDS)
   {
      /* The message failed to complete: */
      status = NI_NO_RESPONSES;
      fprintf(lon_msg_log,"Status=NI_NO_RESPONSES");
   }

//	fprintf(lon_msg_log,"Response to Leave domain is = %02X    Num_Responses=%d \n",response_data.exp.code,num_responses);


    ( void ) handle_error(status, completion, response_data.exp.code,
        "Leave Domain" );           // report any errors

	if (response_data.exp.code== 0x24)
		fprintf(lon_msg_log,"Leave domain WAS A SUCCESS\n");

	else
	{
		if (retry_count < MAX_ERROR_RETRY_COUNT)
		{
			fprintf(lon_msg_log,"Retrying Leave Domain in 2 seconds\n");
			retry_count++;
			sleep(2);
			goto retry_leave_domain;
		
		}
		else
		{
			fprintf(lon_msg_log,"Leave Domain WAS A FAILURE\n");
            return false;
	    }
		
	}


	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//  Function:  update_address
//
//  Author: Karl Hiramoto
//  
//  Date:  16 Aug 2000
//
//  Description:   used to update the neuron address table
//
//
//  Parameters:  address and the address struct used by the neuron
//
//  Return Value:  true on success.   false on error
//
//  Modification Log:
//
//
///////////////////////////////////////////////////////////////////////////////////////////
bool update_address(SendAddrDtl  *send_addr,NM_update_addr_request_struct *NewAddr)
{

	NI_Code      status = NI_OK;
	//SendAddrDtl outAddr;
	//MsgData     outData;
	MsgData     response_data;
	int         response_length;
	ComplType   completion;
	int num_responses=0;
	RespAddrDtl response_addr;
	int	retry_count=0;

    NewAddr->code= NM_update_addr_request_code;

	retry_update_address:
	
	response_data.exp.code= 0; // init


   /* Ship out the message and wait for the response: */
   status = ni_send_msg_wait(REQUEST,
   				  send_addr,  // send to address
    			 (MsgData *) NewAddr,  // send data
				  sizeof(NM_update_addr_request_struct),        // data length
				  false,	// priority
				  false, // send_auth
                  &completion, // completion code
		          &num_responses,  // number of responses
			      &response_addr,	// resp_addr
			      &response_data,  // resp_data
			      &response_length);  // resp length

   if (status == NI_OK && completion != MSG_SUCCEEDS)
   {
      /* The message failed to complete: */
      status = NI_NO_RESPONSES;
      fprintf(lon_msg_log,"Status=NI_NO_RESPONSES");
   }


	if (response_data.exp.code== 0x26)
	{
	//	fprintf(lon_msg_log,"update address WAS A SUCCESS\n");
	    return true;	
	}

	else
	{
//		fprintf(lon_msg_log,"Response Update Address is = %02X    Num_Responses=%d \n",response_data.exp.code,num_responses);


        ( void ) handle_error(status, completion, response_data.exp.code,
        "Update Address" );           // report any errors

		if (retry_count < MAX_ERROR_RETRY_COUNT)
		{
			fprintf(lon_msg_log,"Retrying  in 2 seconds\n");
			retry_count++;
			sleep(2);
			goto retry_update_address;
		
		}
		else
		{
			fprintf(lon_msg_log,"Update Address WAS A FAILURE\n");
            return false;
	    }
		
	}


	return true;
}
// lon_app.c


/*
 ****************************************************************************
 * install_prog_ID - install a program ID into the network interface
 ****************************************************************************
 */

void install_prog_ID( void ) {
    struct {
        NM_write_memory_request     write_mem_hdr;
        char     prog_ID[ ID_STR_LEN ];           // data to be written
    } write_mem_msg = {
         {  NM_write_memory,      // code
            READ_ONLY_RELATIVE,   // mode
            0x00, 0x0D,           // offset for id_string
            ID_STR_LEN,           // count
            BOTH_CS_RECALC },     // form
            "HostApp" };         // program ID

        ComplType       completion;
        MsgData         response;
        NI_Code         ni_error;
        SendAddrDtl     net_intfc_addr;

        net_intfc_addr.lc.type = LOCAL;     // send it to the network intfc
	  //  sleep(1);  // karl test
        ni_error = ni_send_msg_wait(
            REQUEST,
            & net_intfc_addr,         // out_addr
            ( MsgData * ) &write_mem_msg,  // out_data
            sizeof( write_mem_msg ),  // out_length
            false,                    // priority
            false,                    // out_auth
            & completion,             // completion code
            NULL,               // num_responses
            NULL,               // in_addr
            & response,         // in_data
            NULL );             // in_length
	 //   sleep(1);  // karl test

    ( void ) handle_error( ni_error, completion, response.exp.code,
        "updating program ID" );           // report any errors
}

/*************************************************************
 *
 *          Load NV config table from disk
 *
 *************************************************************/

void load_NV_config( void ) {       // called from main() startup code
    int fd;
    unsigned byte_count;

    fd = open( "NVCONFIG.TBL", O_RDONLY );
    if( fd < 0 ) return;        // No file yet
    byte_count = read( fd, nv_config_table, sizeof( nv_config_table ) );
    if( byte_count == sizeof( nv_config_table ) ) {     // success
        fprintf(lon_msg_log, "Network variable config table loaded\n" );
        close( fd );
        return;
    }
    fprintf(lon_msg_log, "Unable to load NV config table from file\n" );
    close( fd );
}

bool update_NV_config(SendAddrDtl  *send_addr,NM_update_nv_cnfg_request *NVUpdate)
{
    ComplType       completion;
    NI_Code ni_error=NI_OK;
    //NI_Code again_ni_error=NI_OK;

	MsgData     response_data;
	//MsgData     send_data;

	int         response_length;
	RespAddrDtl response_addr;
	int num_responses;
	//int send_length=0;
	int retry_count=0;
	
	retry_update_NV_config:   // go here if we need to retry.
	
    ni_error = ni_send_msg_wait(
        	    REQUEST,          /* ACKD, UNACKD_RPT, UNACKD, REQUEST */
        	    send_addr,       /* address of outgoing message */
   (MsgData*)   NVUpdate,       /* data of outgoing message */
        	    sizeof(NM_update_nv_cnfg_request),       /* length of outgoing message */
	            false,         /* outgoing message priority */
    	        false,         /* outgoing message authenticated */
                &completion,       /* MSG_SUCCEEDS or MSG_FAILS */
        	    &num_responses,    /* number of received responses */
    	        &response_addr,          /* address of first response */
    	        &response_data,          /* data of first response */
    	        &response_length);         /* length of first response */



    if (ni_error == NI_OK && completion != MSG_SUCCEEDS)
    {
        /* The message failed to complete: */
        ni_error = NI_NO_RESPONSES;
        fprintf(lon_msg_log,"Status=NI_NO_RESPONSES in  update_NV_config()  Retrying in 2 seconds.\n");

        if (retry_count < MAX_ERROR_RETRY_COUNT)
        { 
            sleep(2);
			retry_count++;
			goto retry_update_NV_config;
		}
		fprintf(lon_msg_log,"\n update_NV_config FAILED returning false \n");
		return false;
    }

	if(ni_error)
		fprintf(lon_msg_log,"ERROR in update_NV_config() \n");

	if (response_data.exp.code== 0x2B)
    {
	//	fprintf(lon_msg_log,"update_NV_config WAS A SUCCESS\n");
	}
	else
	{
        fprintf(lon_msg_log,"\nupdate_NV_config()  Response is = %02X  Num_Responses=%d \n",response_data.exp.code,num_responses);
		fprintf(lon_msg_log,"update_NV_config WAS A FAILURE \n");
		
        ( void ) handle_error(ni_error, completion, response_data.exp.code,
        "update_NV_config" );           // report any errors
	    return false;
	}

    return(true);
        
}



/////////////////////////////////////////////////////////////////////////////////////////////
//  Function: clear_address
//
//  Author: Karl Hiramoto
//  
//  Date:  16 Aug 2000
//
//  Description: used on neruon programming to clear out the address table.  
//               Basicly send a bunch of 0's to the address able 
//
//
//  Parameters:  the address of the neuron ID to send to.
//
//  Return Value:   true on sucess.  false on error.
//
//  Modification Log:
//
//
///////////////////////////////////////////////////////////////////////////////////////////
bool clear_address_table(SendAddrDtl  *send_addr)
{

    NM_update_addr_request_struct  NewEmptyAddr;

    NewEmptyAddr.code= NM_update_addr_request_code;
    NewEmptyAddr.type=0;
    NewEmptyAddr.domain=0;
    NewEmptyAddr.member_or_node=0;
    NewEmptyAddr.rpt_timer=0;
    NewEmptyAddr.retry=0;
    NewEmptyAddr.rcv_timer=0;
    NewEmptyAddr.tx_timer=0;
    NewEmptyAddr.group_or_subnet=0;

    for (int i=0 ; i <= 0x0E ; i++)
    {
        NewEmptyAddr.addr_index=i;
	    if (!update_address(send_addr,&NewEmptyAddr))
	    {
	        fprintf(lon_msg_log,"UPDATE ADDRESS FAILED;");
	        return false;
		}
	
	}
   // fprintf(lon_msg_log,"UPDATE ADDRESS SUCCESS");
    return true;
}


/////////////////////////////////////////////////////////////////////////////////////////////
//  Function:    update_domain
//
//  Author: Karl Hiramoto
//  
//  Date:  16 Aug 2000
//
//  Description:    used to update the neruon domain table.
//
//
//  Parameters: the address to send to, and the domain struct defined by the neuron.
//
//  Return Value:  true on succes.  false on error.
//
//  Modification Log:
//
//
///////////////////////////////////////////////////////////////////////////////////////////
bool update_domain(SendAddrDtl  *send_addr,NM_update_domain_request_struct *NewDomain)
{

	NI_Code      status = NI_OK;
//	SendAddrDtl outAddr;
//	MsgData     outData;
	MsgData     response_data;
	int         response_length;
	ComplType   completion;
	int num_responses=0;
	RespAddrDtl response_addr;
	int	retry_count=0;

    NewDomain->code= NM_update_domain_request_code;

	retry_update_domain:
	
	response_data.exp.code= 0; // init


   /* Ship out the message and wait for the response: */
   status = ni_send_msg_wait(REQUEST,
   				  send_addr,  // send to address
    			 (MsgData *) NewDomain,  // send data
				  sizeof(NM_update_domain_request_struct),        // data length
				  false,	// priority
				  false, // send_auth
                  &completion, // completion code
		          &num_responses,  // number of responses
			      &response_addr,	// resp_addr
			      &response_data,  // resp_data
			      &response_length);  // resp length

   if (status == NI_OK && completion != MSG_SUCCEEDS)
   {
      /* The message failed to complete: */
      status = NI_NO_RESPONSES;
      fprintf(lon_msg_log,"Status=NI_NO_RESPONSES");
   }

//fprintf(lon_msg_log,"Response update domain is = %02X    Num_Responses=%d \n",response_data.exp.code,num_responses);


    ( void ) handle_error(status, completion, response_data.exp.code,
        "Update Address" );           // report any errors

	if (response_data.exp.code== 0x23)
		fprintf(lon_msg_log,"update domain WAS A SUCCESS\n");

	else
	{
		if (retry_count < MAX_ERROR_RETRY_COUNT)
		{
			fprintf(lon_msg_log,"Retrying  in 2 seconds\n");
			retry_count++;
			sleep(2);
			goto retry_update_domain;
		
		}
		else
		{
			fprintf(lon_msg_log,"Update domain WAS A FAILURE\n");
            return false;
	    }
		
	}


	return true;
}


/////////////////////////////////////////////////////////////////////////////////////////////
//  Function:
//
//  Author: Karl Hiramoto
//  
//  Date:  16 Aug 2000
//
//  Description:
//
//
//  Parameters:
//
//  Return Value:
//
//  Modification Log:
//
//
///////////////////////////////////////////////////////////////////////////////////////////
bool init_domain(SendAddrDtl  *send_addr,byte subnet, byte node)
{

    NM_update_domain_request_struct SendNewDomain;

    SendNewDomain.code=NM_update_domain_request_code;
    SendNewDomain.domain_index=0;
    SendNewDomain.id[0]=0x44; //  == '\'
    
    for (int i=1 ; i < DOMAIN_ID_LEN; i++)
        SendNewDomain.id[i]=0;
	
//    SendNewDomain.subnet=0;

    SendNewDomain.subnet=subnet;
    
//    SendNewDomain.must_be_one=0;  // treat as a cloned node.  was 0 for motorola lon chip
    SendNewDomain.must_be_one=1;

    SendNewDomain.node=node;  // was 5.     probally should be unique.  see data book for explanation.

    SendNewDomain.len=1;
    for (int i=0 ; i < AUTH_KEY_LEN; i++)
        SendNewDomain.key[i]=0xff;
    

    return update_domain(send_addr,&SendNewDomain);

}


/////////////////////////////////////////////////////////////////////////////////////////////
//  Function:
//
//  Author: Karl Hiramoto
//  
//  Date:  16 Aug 2000
//
//  Description:
//
//
//  Parameters:
//
//  Return Value:
//
//  Modification Log:
//
//
///////////////////////////////////////////////////////////////////////////////////////////
bool clear_status(SendAddrDtl  *send_addr)
{

	NI_Code      status = NI_OK;
	//SendAddrDtl outAddr;
	MsgData     outData;
	MsgData     response_data;
	int         response_length;
	ComplType   completion;
	int num_responses=0;
	RespAddrDtl response_addr;
	int	retry_count=0;


	outData.exp.code = NM_clear_status;

	retry_clear_status:
	
	response_data.exp.code= 0; // init


   /* Ship out the message and wait for the response: */
   status = ni_send_msg_wait(REQUEST,
   				  send_addr,  // send to address
    			 &outData,  // send data
				  1,        // data length
				  false,	// priority
				  false, // send_auth
                  &completion, // completion code
		          &num_responses,  // number of responses
			      &response_addr,	// resp_addr
			      &response_data,  // resp_data
			      &response_length);  // resp length

   if (status == NI_OK && completion != MSG_SUCCEEDS)
   {
      /* The message failed to complete: */
      status = NI_NO_RESPONSES;
      fprintf(lon_msg_log,"Status=NI_NO_RESPONSES");
   }




	if (response_data.exp.code== 0x33)
    {
    
    //		fprintf(lon_msg_log,"Clear Status WAS A SUCCESS\n");
            return true;
	}

	else
	{
	
    	fprintf(lon_msg_log,"Response to Clear Status is = %02X    Num_Responses=%d \n",response_data.exp.code,num_responses);
	
	    ( void ) handle_error(status, completion, response_data.exp.code,
        "Query status" );           // report any errors

		if (retry_count < MAX_ERROR_RETRY_COUNT)
		{
			fprintf(lon_msg_log,"Retrying error in 2 seconds\n");
			retry_count++;
			sleep(2);
			goto retry_clear_status;
		
		}
		else
		{
			fprintf(lon_msg_log,"Clear Status WAS A FAILURE\n");
            return false;
	    }
		
	}


	return true;
}


/////////////////////////////////////////////////////////////////////////////////////////////
//  Function:
//
//  Author: Karl Hiramoto
//  
//  Date:  16 Aug 2000
//
//  Description:
//
//
//  Parameters:
//
//  Return Value:
//
//  Modification Log:
//
//
///////////////////////////////////////////////////////////////////////////////////////////
bool query_status(SendAddrDtl  *send_addr, status_struct *NeuronStatusPtr )
{

	NI_Code      status = NI_OK;
	//SendAddrDtl outAddr;
	MsgData     outData;
	MsgData     response_data;
	int         response_length;
	ComplType   completion;
	int num_responses=0;
	RespAddrDtl response_addr;
	int	retry_count=0;


	outData.exp.code = NM_query_status;

	retry_query_status:
    response_data.exp.code=0; /// init for retry

   /* Ship out the message and wait for the response: */
   status = ni_send_msg_wait(REQUEST,
   				  send_addr,  // send to address
				 &outData,  // send data
				  1,        // data length
				  false,	// priority
				  false, // send_auth
                  &completion, // completion code
		          &num_responses,  // number of responses
			      &response_addr,	// resp_addr
                  &response_data,
//			      (MsgData*) NeuronStatusPtr,  // resp_data
			      &response_length);  // resp length

   if (status == NI_OK && completion != MSG_SUCCEEDS)
   {
      /* The message failed to complete: */
      status = NI_NO_RESPONSES;
      fprintf(lon_msg_log,"Status=NI_NO_RESPONSES query_status()\n");
   }



	if (response_data.exp.code== 0x31)
	{
//		fprintf(lon_msg_log,"QUERY WAS A SUCCESS\n");
        memcpy(NeuronStatusPtr,&response_data.exp.data[0],response_length-1);
	}

	else
	{
		fprintf(lon_msg_log,"QUERY WAS A FAILURE\n");
		fprintf(lon_msg_log,"Response code to query is = %02X    Num_Responses=%d \n",response_data.exp.code,num_responses);

        ( void ) handle_error(status, completion, response_data.exp.code,
                "Query status" );           // report any errors
		
		if (retry_count < MAX_ERROR_RETRY_COUNT)
		{
			fprintf(lon_msg_log,"Retrying error in 2 seconds\n");
			retry_count++;
			sleep(2);
			goto retry_query_status;
		
		}
		else
			return false;
		
	}


	return true;
}


/////////////////////////////////////////////////////////////////////////////////////////////
//  Function:
//
//  Author: Karl Hiramoto
//  
//  Date:  16 Aug 2000
//
//  Description:
//
//
//  Parameters:
//
//  Return Value:
//
//  Modification Log:
//
//
///////////////////////////////////////////////////////////////////////////////////////////

// Used to set the node mode and state how you want it.
bool change_node_mode_state(SendAddrDtl  *send_addr,byte set_mode, byte set_state)
{
    ComplType       completion;
    NI_Code ni_error=NI_OK;
    //NI_Code again_ni_error=NI_OK;

	MsgData     response_data;
	MsgData     send_data;

	int         response_length;
	RespAddrDtl response_addr;
//	NM_set_node_mode_request  NodeModeStateRequest;
	int num_responses;
	int send_length=0;
	int retry_count=0;
	ServiceType  TypeOfMsg;
		                        
					
					// for check again test.
    //ServiceType again_service;
    //RespAddrDtl  again_in_addr;
    //MsgData again_in_data;
    //int again_in_length;
    //bool again_in_auth;					
					
//	NodeModeStateRequest.code=NM_set_node_mode; // 0x6C
//	NodeModeStateRequest.mode=set_mode;
//	NodeModeStateRequest.node_state=set_state;

    send_data.exp.code=NM_set_node_mode; // 0x6C
    send_data.exp.data[0]=set_mode;
    send_data.exp.data[1]=set_state;


	if (set_state==0)
		send_length=2;  // do not include state variable
	else 
		send_length=3;  // include state variable


	if (set_state == APPL_UNCNFG ||  set_state==NO_APPL_UNCNFG ||
          set_state==CNFG_ONLINE || set_state==CNFG_OFFLINE)
	    TypeOfMsg=REQUEST;

    else
        TypeOfMsg=ACKD;

   
   if (set_mode ==APPL_RESET) // no response from reset. must query
        TypeOfMsg=UNACKD;
   
	retry_count=0;
	
	retry_node_state:   // go here if we need to retry.
	response_data.exp.code=0;
	
        // was UUNACKED
    ni_error = ni_send_msg_wait(
	    TypeOfMsg,          /* ACKD, UNACKD_RPT, UNACKD, REQUEST */
	    send_addr,       /* address of outgoing message */
        &send_data,       /* data of outgoing message */
//	    (MsgData *) &NodeModeStateRequest,       /* data of outgoing message */
	    send_length,       /* length of outgoing message */
	    false,         /* outgoing message priority */
	    false,         /* outgoing message authenticated */
        &completion,       /* MSG_SUCCEEDS or MSG_FAILS */
	    &num_responses,    /* number of received responses */
	    &response_addr,          /* address of first response */
	    &response_data,          /* data of first response */
	    &response_length);         /* length of first response */



   if (ni_error == NI_OK && completion != MSG_SUCCEEDS && TypeOfMsg==REQUEST)
   {
      /* The message failed to complete: */
      ni_error = NI_NO_RESPONSES;
      fprintf(lon_msg_log,"Status=NI_NO_RESPONSES in change_mode_state() \n");
      fprintf(lon_msg_log,"set_mode= %d  ;  set_state = %d   RETRYING in 2 seconds\n", set_mode, set_state);
      fprintf(lon_msg_log,"\nchange_node_mode_state()  Response is = %02X  Num_Responses=%d \n",response_data.exp.code,num_responses);


		if (retry_count < MAX_ERROR_RETRY_COUNT)
		{ 
			sleep(2);
			retry_count++;
			goto retry_node_state;
		}
		return false;
   } else if (TypeOfMsg==UNACKD)
        return true;
    else if (TypeOfMsg==ACKD)
        return true;  // received ack.


	if(ni_error)
		fprintf(lon_msg_log,"ERROR in set_node_applicationless() \n");


//	fprintf(lon_msg_log,"\nchange_node_mode_state()  Response is = %02X  Num_Responses=%d \n",response_data.exp.code,num_responses);

	if (response_data.exp.code== 0x2C)
		fprintf(lon_msg_log,"Change state WAS A SUCCESS\n");
	else
	{
		fprintf(lon_msg_log,"Change state WAS A FAILURE -- no response needed for reset, online, offline\n");
	}
	
    
    ( void ) handle_error(ni_error, completion, response_data.exp.code,
        "updating program ID" );           // report any errors

return(true);
}

/////////////////////////////////////////////////////////////////////////////////////////////
//  Function:
//
//  Author: Karl Hiramoto
//  
//  Date:  16 Aug 2000
//
//  Description:  Reset the node
//
//
//  Parameters:
//
//  Return Value:
//
//  Modification Log:
//
//
///////////////////////////////////////////////////////////////////////////////////////////
bool reset_node(SendAddrDtl  *send_addr)
{
    int retry_count;

    status_struct NeuronStatus;
    
//    fprintf(lon_msg_log,"RESETING NODE in reset_node()\n");

    if (!query_status(send_addr,&NeuronStatus ))
    {   // error
        fprintf(lon_msg_log,"error in reset_node.  query_status failed\n");
	    return false;
    }

//    fprintf(lon_msg_log,"Status Before Clear_status");
//    print_neuron_status_struct(&NeuronStatus);

        
    if (!clear_status(send_addr))
    {   // error
        fprintf(lon_msg_log,"error in reset_node.  clear_status failed\n");
	    return false;
    }

    sleep(1);

// verify that status is clear.
    if (!query_status(send_addr,&NeuronStatus ))
    {   // error
        fprintf(lon_msg_log,"error in reset_node.  query_status failed\n");
	    return false;
    }
    
//    fprintf(lon_msg_log,"After clear status\n");
//    print_neuron_status_struct(&NeuronStatus);

//    if (NeuronStatus.reset_cause != 0)
//    {
//        fprintf(lon_msg_log,"Node clear status not properly reset.. exit..\n");
//	    return false;
//    }
        
	
	 retry_count=0;
	 
	 retry_reset:    
	 

// reset the node
    if(!change_node_mode_state(send_addr,APPL_RESET, 0))
    {   // error
        fprintf(lon_msg_log,"error in reset_node.  change node mode_state failed.\n");
	    return false;
    }
    
    sleep(1);

// verify that the node was reset.
    if(!query_status(send_addr,&NeuronStatus ))
    {   // error
        fprintf(lon_msg_log,"error in reset_node.  query_status failed\n");
	    return false;
    }

//    fprintf(lon_msg_log,"After sending reset the status is:\n");
//    print_neuron_status_struct(&NeuronStatus);


    if(NeuronStatus.reset_cause & 0x14) //  if was a software reset it was a success
    {
//        fprintf(lon_msg_log,"RESET SUCCESS\n");
        return true;
    
    }    
    else if (retry_count < MAX_ERROR_RETRY_COUNT)
	{ 
		++retry_count;
		fprintf(lon_msg_log,"Error on RESET.  Retrying the reset in 1 second.\n");
		sleep(1);
		goto retry_reset;
	}
    else
       
    fprintf(lon_msg_log,"RESET FAILURE in reset_node() \n");
    return false;
}


/////////////////////////////////////////////////////////////////////////////////////////////
//  Function:
//
//  Author: Karl Hiramoto
//  
//  Date:  16 Aug 2000
//
//  Description:
//
//
//  Parameters:
//
//  Return Value:
//
//  Modification Log:
//
//
///////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////
//  Function:
//
//  Author: Karl Hiramoto
//  
//  Date:  16 Aug 2000
//
//  Description:
//
//
//  Parameters:
//
//  Return Value:
//
//  Modification Log:
//
//
///////////////////////////////////////////////////////////////////////////////////////////

//long long convert_byte_array_to_int(byte *Arr,int length)
long long convert_byte_array_to_int(const void *data,int length)
{

	long long temp=0;
    int i;
    byte * ptr;

    for( i = 0, ptr = ( byte * ) data; i < length; i++, ptr++ )
	{
		temp=temp << 8; // shift one byte
		temp = temp | (long long) *ptr;
	}

	return temp;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//  Function:
//
//  Author: Karl Hiramoto
//  
//  Date:  16 Aug 2000
//
//  Description:
//
//
//  Parameters:
//
//  Return Value:
//
//  Modification Log:
//
//
///////////////////////////////////////////////////////////////////////////////////////////
void convert_int_to_byte_array(long long Input, byte *Arr,byte length)
{
	const long long Mask= 0x000000000000FF;
	byte *TempPtr;
	int i;
	
	TempPtr=Arr+length; // Start ptr at end of array
	
//	fprintf(lon_msg_log,"Original Input = %016llX \n",Input);
	for( i=0 ; i < length; i++)
	{
		TempPtr--;  // start one back from length because item 8 = item[7]

		*TempPtr= (byte) (Input & Mask);
///		fprintf(lon_msg_log,"byte %d  is = %02X \n",i,*TempPtr);
		Input = Input >> 8; 
	}

}

/////////////////////////////////////////////////////////////////////////////////////////////
//  Function:
//
//  Author: Karl Hiramoto
//  
//  Date:  16 Aug 2000
//
//  Description:
//
//
//  Parameters:
//
//  Return Value:
//
//  Modification Log:
//
//
///////////////////////////////////////////////////////////////////////////////////////////

void print_all_nodes(void)
{
/*
	char Prog_ID[ID_STR_LEN+1];
	byte ByteArr[ID_STR_LEN+1];
	
	Prog_ID[ID_STR_LEN]=0; // null terminate

    fprintf(lon_msg_log,"Selection | Neuron ID(Hex) | Program ID(Hex)  | Program ID(ASCII)\n");

	for (uint i=0 ; i < NumNetworkNodes; i++)
	{
		convert_int_to_byte_array(NetworkNodeTable[i].Program_ID,ByteArr,ID_STR_LEN);
		
		strncpy(&Prog_ID[0],(char *) &ByteArr[0],ID_STR_LEN);
		printw ("%d   =     |  %012llX  | %016llX | %s\n",i,NetworkNodeTable[i].Neuron_ID,NetworkNodeTable[i].Program_ID,Prog_ID);
	}
*/

}



/////////////////////////////////////////////////////////////////////////////////////////////
//  Function:
//
//  Author: Karl Hiramoto
//  
//  Date:  16 Aug 2000
//
//  Description:
//
//
//  Parameters:
//
//  Return Value:
//
//  Modification Log:
//
//
///////////////////////////////////////////////////////////////////////////////////////////

void list_nodes(void)
{	fprintf(lon_msg_log,"Current Nodes on the Network : \n");
	print_all_nodes();
}

/////////////////////////////////////////////////////////////////////////////////////////////
//  Function:
//
//  Author: Karl Hiramoto
//  
//  Date:  16 Aug 2000
//
//  Description:
//
//
//  Parameters:
//
//  Return Value:
//
//  Modification Log:
//
//
///////////////////////////////////////////////////////////////////////////////////////////
unsigned short int get_node_choice(void)
{


	//int i=0;
	int choice=0;
    int valid=0;
    int junk;
	if (NumNetworkNodes==0)
		return 0;  // nothing to choose from.


	fprintf(lon_msg_log,"Which node would you like to act on? : \n");

	print_all_nodes();

		
	fprintf(lon_msg_log,"Make your selection --> ");
    
    valid=scanf("%d",&choice);
    
    while(!valid)
    {
        junk=getchar(); // empty out stidin
    	fprintf(lon_msg_log,"ERROR: Invalid.  Type Numeric Number. Please choose again-->");
	    valid=scanf("%d",&choice);

	}
	
	fprintf(lon_msg_log,"DEBUG..  You selected choice =%d \n",choice);
	
	if ((uint)choice >= NumNetworkNodes)
	{
	   	fprintf(lon_msg_log,"ERROR:  Invalid choice.  Please try again\n");
		choice=get_node_choice(); // Invalid choice  get another selection.
	}
	
    return (unsigned short int) choice;

}

/////////////////////////////////////////////////////////////////////////////////////////////
//  Function:
//
//  Author: Karl Hiramoto
//  
//  Date:  16 Aug 2000
//
//  Description:
//
//
//  Parameters:
//
//  Return Value:
//
//  Modification Log:
//
//
///////////////////////////////////////////////////////////////////////////////////////////
unsigned short int get_nv_choice(void)
{
	//int i=0;
	int choice=0;

	fprintf(lon_msg_log,"Which NV would you like to act on? : \n");

	fprintf(lon_msg_log,"Make your selection --> ");
    
    while(!scanf("%d",&choice))
    	fprintf(lon_msg_log,"ERROR: Invalid.  Type Numeric Number. Please choose again-->");
	
	fprintf(lon_msg_log,"DEBUG..  You selected choice =%d \n",choice);
	
    return (unsigned short int) choice;

}


/////////////////////////////////////////////////////////////////////////////////////////////
//  Function:
//
//  Author: Karl Hiramoto
//
//  Date:  16 Aug 2000
//
//  Description:
//
//
//  Parameters:
//
//  Return Value:
//
//  Modification Log:
//
//
///////////////////////////////////////////////////////////////////////////////////////////
void setup_NID_addrress(SendAddrDtl *send_addr,NeuronCalibrationItem  *CT)
{

	send_addr->id.type=NEURON_ID;
	send_addr->id.rsvd0=0;
	send_addr->id.domain=0;
//	send_addr->id.retry=0;
//	send_addr->id.rpt_timer=0;
//	send_addr->id.tx_timer=0;

	send_addr->id.retry=MAX_ERROR_RETRY_COUNT;
	send_addr->id.rpt_timer=12; // 1024 ms to repeat a unack_rpt msg
	send_addr->id.tx_timer=12; // 778 ms is the max time to wait

	send_addr->id.rsvd2=0;
	send_addr->id.subnet=0;
	convert_int_to_byte_array(CT->Neuron_ID,send_addr->id.nid,sizeof(send_addr->id.nid));


//    fprintf(lon_msg_log,"NID address is =");
//    print_array(send_addr->id.nid,sizeof(send_addr->id.nid),"%02X ");
//    fprintf(lon_msg_log,"\n");

}

/////////////////////////////////////////////////////////////////////////////////////////////
//  Function:
//
//  Author: Karl Hiramoto
//  
//  Date:  16 Aug 2000
//
//  Description:
//
//
//  Parameters:
//
//  Return Value:
//
//  Modification Log:
//
//
///////////////////////////////////////////////////////////////////////////////////////////
bool read_memory(SendAddrDtl  *send_addr,nm_mem_mode read_mode,word offset,byte count,MsgData *response_data)
{

	NI_Code     status = NI_OK;
	//SendAddrDtl outAddr;
	int         response_length;
	ComplType   completion;
	int num_responses=0;
	RespAddrDtl response_addr;
	NM_read_memory_request  ReadMemoryRequest;
	int retry_count=0;
	
	ReadMemoryRequest.code=NM_read_memory;
	ReadMemoryRequest.mode=(byte) read_mode;
	ReadMemoryRequest.offset_hi= (byte) (offset >> 8); // get 8 MSB's
	ReadMemoryRequest.offset_lo= (byte) (offset & 0x00ff);
	ReadMemoryRequest.count=count;

	retry_count=0;	

	retry_read:
   response_data->exp.code=0; // init for check later;
   
	/* Ship out the message and wait for the response: */
	status = ni_send_msg_wait(REQUEST,
   				 send_addr,  // send to address
				 (MsgData*) &ReadMemoryRequest,  // send data
				  sizeof(ReadMemoryRequest),        // data length
				  false,	// priority
				  false, // send_auth
                  &completion, // completion code
		          &num_responses,  // number of responses
			      &response_addr,	// resp_addr
			       response_data,  // resp_data
			      &response_length);  // resp length

	if (response_data->exp.code!= 0x2D)
	{
		// there was an error on read.  Retry.
		
		if (retry_count < MAX_ERROR_RETRY_COUNT)
		{ 
			++retry_count;
			fprintf(lon_msg_log,"Error on read.  Retrying the read in 1 second.\n");
			sleep(1);
			goto retry_read;
		
		}
	}

   if (status == NI_OK && completion != MSG_SUCCEEDS)
   {
      /* The message failed to complete: */
      status = NI_NO_RESPONSES;
      fprintf(lon_msg_log,"Status=NI_NO_RESPONSES read_memory()\n");
   }

//	fprintf(lon_msg_log,"Response code to Read_memory is = %02X    Num_Responses=%d \n",response_data->exp.code,num_responses);

    ( void ) handle_error(status, completion, response_data->exp.code,
        "Reading Memory" );           // report any errors


	if (response_data->exp.code== 0x2D)
	{
//		fprintf(lon_msg_log,"Read Mem WAS A SUCCESS\n");
		return true;
	
	}
	else
	{
		fprintf(lon_msg_log,"Read Mem WAS A FAILURE\n");
		return false;
	}

}



// read up to 8 bytes
long long read_neuron_memory_long_long(NeuronCalibrationItem *CTSPtr,nm_mem_mode read_mode,word offset,byte count)
{
    MsgData response_data;
    SendAddrDtl send_addr;


	setup_NID_addrress(&send_addr,CTSPtr);

    if (count > 8)
        count=8;

    if (read_memory(&send_addr,read_mode,offset,count,&response_data))
        return convert_byte_array_to_int(&response_data.exp.data[0],count);
    else return 0;
    
}

/////////////////////////////////////////////////////////////////////////////////////////////
//  Function:
//
//  Author: Karl Hiramoto
//  
//  Date:  16 Aug 2000
//
//  Description:
//
//
//  Parameters:
//
//  Return Value:
//
//  Modification Log:
//
//
///////////////////////////////////////////////////////////////////////////////////////////
bool fetch_nv_cnfg(SendAddrDtl  *send_addr,byte nv_index,nv_struct *response_nv_struct)
{

	NI_Code     status = NI_OK;
	MsgData     outData;
	//SendAddrDtl outAddr;
	int         response_length;
	ComplType   completion;
	int num_responses=0;
	RespAddrDtl response_addr;
	NM_query_nv_cnfg_response  response_data;
	int retry_count=0;
	
	outData.exp.code=NM_query_nv_cnfg;
	outData.exp.data[0]=nv_index;

	retry_count=0;	

	retry_fetch_nv_cnfg:
	
	response_data.code=0; // init for check later;
   
	/* Ship out the message and wait for the response: */
	status = ni_send_msg_wait(REQUEST,
   				 send_addr,  // send to address
				 &outData,  // send data
				  2,        // data length
				  false,	// priority
				  false, // send_auth
                  &completion, // completion code
		          &num_responses,  // number of responses
			      &response_addr,	// resp_addr
				(MsgData*)   &response_data,  // resp_data
			      &response_length);  // resp length

	if (response_data.code!=NM_query_nv_cnfg_succ)
	{
		// there was an error on read.  Retry.
		
		if (retry_count < MAX_ERROR_RETRY_COUNT)
		{ 
			++retry_count;
			fprintf(lon_msg_log,"Error on fetch_nv_cnvg().  Retrying the fetch in 1 second.\n");
			sleep(1);
			goto retry_fetch_nv_cnfg;
		
		}
	}

   if (status == NI_OK && completion != MSG_SUCCEEDS)
   {
      /* The message failed to complete: */
      status = NI_NO_RESPONSES;
      fprintf(lon_msg_log,"Status=NI_NO_RESPONSES fetch_nv_config()\n");
   }


    ( void ) handle_error(status, completion, response_data.code,
        "Fetch Network Variable Config" );           // report any errors


	if (response_data.code== NM_query_nv_cnfg_succ)
	{
//		fprintf(lon_msg_log,"fetch_nv_cnfg WAS A SUCCESS\n");
		memcpy(response_nv_struct,&response_data.nv_cnfg,sizeof(nv_struct));
		return true;  // two is for code and index
	
	}
	else
	{
		fprintf(lon_msg_log,"fetch_nv_cnfg WAS A FAILURE\n");
		fprintf(lon_msg_log,"Response code to fetch_nv_cnfg is = %02X  Num_Responses=%d length=%d\n",response_data.code,num_responses,response_length);

		return false;
	}


}

/////////////////////////////////////////////////////////////////////////////////////////////
//  Function:
//
//  Author: Karl Hiramoto
//  
//  Date:  16 Aug 2000
//
//  Description:
//
//
//  Parameters:
//
//  Return Value:  returns 0 or error.  On success it returns the length(bytes) of the NV
//
//  Modification Log:
//
//
///////////////////////////////////////////////////////////////////////////////////////////
int fetch_nv(SendAddrDtl  *send_addr,byte nv_index,MsgData *response_data)
{

	NI_Code     status = NI_OK;
	MsgData     outData;
	//SendAddrDtl outAddr;
	int         response_length;
	ComplType   completion;
	int num_responses=0;
	RespAddrDtl response_addr;
	int retry_count=0;
	
	outData.exp.code=NM_NV_fetch;
	outData.exp.data[0]=nv_index;

	retry_count=0;	

	retry_fetch_nv:
	response_data->exp.code=0; // init for check later;
   
	/* Ship out the message and wait for the response: */
	status = ni_send_msg_wait(REQUEST,
   				 send_addr,  // send to address
				 &outData,  // send data
				  2,        // data length
				  false,	// priority
				  false, // send_auth
                  &completion, // completion code
		          &num_responses,  // number of responses
			      &response_addr,	// resp_addr
			       response_data,  // resp_data
			      &response_length);  // resp length

	if (response_data->exp.code!= 0x33)
	{
		// there was an error on read.  Retry.
		
		if (retry_count < MAX_ERROR_RETRY_COUNT)
		{ 
			++retry_count;
			fprintf(lon_msg_log,"Error on fetch_nv().  Retrying the fetch in 1 second.\n");
			sleep(1);
			goto retry_fetch_nv;
		
		}
	}

   if (status == NI_OK && completion != MSG_SUCCEEDS)
   {
      /* The message failed to complete: */
      status = NI_NO_RESPONSES;
      fprintf(lon_msg_log,"Status=NI_NO_RESPONSES fetch_nv()\n");
   }


//    ( void ) handle_error(status, completion, response_data->exp.code,
//        "Fetch Network Variable" );           // report any errors


	if (response_data->exp.code== 0x33)
	{
//		fprintf(lon_msg_log,"fetch_nv WAS A SUCCESS\n");
		return response_length-2;  // two is for code and index
	
	}
	else
	{
		fprintf(lon_msg_log,"fetch_nv WAS A FAILURE\n");
		fprintf(lon_msg_log,"Response code to fetch_nv is = %02X  Num_Responses=%d length=%d\n",response_data->exp.code,num_responses,response_length);

		return 0;
	}


}

/////////////////////////////////////////////////////////////////////////////////////////////
//  Function:
//
//  Author: Karl Hiramoto
//  
//  Date:  16 Aug 2000
//
//  Description:
//
//
//  Parameters:
//
//  Return Value:
//
//  Modification Log:
//
//
///////////////////////////////////////////////////////////////////////////////////////////
bool write_nv(SendAddrDtl  *send_addr,byte nv_index,byte length,long long value)
{

	NI_Code     status = NI_OK;
	MsgData     outData;
	//SendAddrDtl outAddr;
	int         response_length;
	ComplType   completion;
	int num_responses=0;
	RespAddrDtl response_addr;
	MsgData		response_data;
	int retry_count=0;
	nv_struct nv_info_response;
	


	if (!fetch_nv_cnfg(send_addr,nv_index,&nv_info_response))
	{
		fprintf(lon_msg_log,"ERROR: could not fetch_nv_cnfg data. in write_nv()\n");


	}
 
						
	outData.exp.code= 0x80 | nv_info_response.selector_hi;  // MSB set.  second from msb is 0 (input) page 9-53 of Motorola Lonworks book
	outData.exp.data[0]=nv_info_response.selector_lo;

	// the rest of the outgoing message data is the network variable value. High byte first
	convert_int_to_byte_array(value,&outData.exp.data[1],length);

	
	retry_count=0;	

	retry_write_nv:
	response_data.exp.code=0; // init for check later;
   
	/* Ship out the message and wait for the response: */
	status = ni_send_msg_wait(ACKD,
   				 send_addr,  // send to address
				 &outData,  // send data
				  length+2,        // data length    2 + number of bytes the NV is
				  false,	// priority
				  false, // send_auth
                  &completion, // completion code
		          &num_responses,  // number of responses
			      &response_addr,	// resp_addr
			      &response_data,  // resp_data
			      &response_length);  // resp length

/*
	if (response_data.exp.code!= 0x33)
	{
		// there was an error on read.  Retry.
		
		if (retry_count < MAX_ERROR_RETRY_COUNT)
		{ 
			++retry_count;
			fprintf(lon_msg_log,"Error on fetch_nv().  Retrying the fetch in 1 second.\n");
			sleep(1);
			goto retry_fetch_nv;
		
		}
	}
*/

   if (status == NI_OK && completion != MSG_SUCCEEDS)
   {
      /* The message failed to complete: */
        status = NI_NO_RESPONSES;
      
	    if (retry_count < MAX_ERROR_RETRY_COUNT)
		{ 
			++retry_count;
			fprintf(lon_msg_log,"Error on write_nv().  Retrying the fetch in 2 seconds.\n");
			sleep(2);
			goto retry_write_nv;
		
		}

   }

//	fprintf(lon_msg_log,"Response code to write_nv is = %02X  Num_Responses=%d length=%d\n",response_data.exp.code,num_responses,response_length);

    ( void ) handle_error(status, completion, response_data.exp.code,
        "Write Network Variable" );           // report any errors


	return true;
}
////////////////////////////////////////////////////////////////////////////////////
bool get_num_net_vars(NeuronCalibrationItem *NCIPtr,int *num)
{

	SendAddrDtl  send_addr;
	MsgData     response_data;
	
	setup_NID_addrress(&send_addr,NCIPtr);
	
			
//get # of network variables.  If >0 then find infon on them.
	if(read_memory(&send_addr,READ_ONLY_RELATIVE,0x0A,1,&response_data))
	{
	    *num=response_data.exp.data[0];
	    return true;
	}
	else 
	{
        //error
        *num=-1;
        return false;
	}

}

bool Get_NV_Config(NeuronCalibrationItem *NCIPtr, byte nv_index,	nv_struct   *nvStructData)
{
	SendAddrDtl  send_addr;
	setup_NID_addrress(&send_addr,NCIPtr);
	

    return fetch_nv_cnfg(&send_addr,nv_index,nvStructData);
    /*
			{
				
				if(nvStructData.priority)
					fprintf(lon_msg_log,"nv_priority=YES      ;  ");
				else
					fprintf(lon_msg_log,"nv_priority=NO       ;  ");

				if(nvStructData.direction)
					fprintf(lon_msg_log,"nv_direction=1 (OUTPUT) ");
				else
					fprintf(lon_msg_log,"nv_direction=0 (INPUT)  ");
				
				fprintf(lon_msg_log,"\n");
				
				fprintf(lon_msg_log,"nv_selector_hi = %02x ; nv_selector_lo = %02x  ; combined=%02X%02X\n",nvStructData.selector_hi,nvStructData.selector_lo,nvStructData.selector_hi,nvStructData.selector_lo);

				if(nvStructData.service==0)
					fprintf(lon_msg_log,"Service=ACKD         ; ");
				else if(nvStructData.service==1)
					fprintf(lon_msg_log,"Service=UNACKD_RPT   ; ");
				else if(nvStructData.service==2)
					fprintf(lon_msg_log,"Service=UNACKD       ; ");
				else
					fprintf(lon_msg_log,"Service= (UNKNOWN) ; ");
			
				
				if(nvStructData.auth)
					fprintf(lon_msg_log,"nv_auth=YES  ; ");
				else
					fprintf(lon_msg_log,"nv_auth=NO   ; ");
					
				if(nvStructData.turnaround)
					fprintf(lon_msg_log,"nv_turnaround=YES ; ");
				else
					fprintf(lon_msg_log,"nv_turnaround=NO  ; ");
		
				fprintf(lon_msg_log,"nv_addr_index=%d \n",nvStructData.addr_index);

			}
*/
}

/////////////////////////////////////////////////////////////////////////////////////////////
//  Function:
//
//  Author: Karl Hiramoto
//  
//  Date:  16 Aug 2000
//
//  Description:
//
//
//  Parameters:
//
//  Return Value:
//
//  Modification Log:
//
//
///////////////////////////////////////////////////////////////////////////////////////////
void query_node(NeuronCalibrationItem *NCIPtr)
{

	SendAddrDtl  send_addr;
	unsigned short int choice=0;
	//SendAddrDtl outAddr;
	MsgData		nvMsgData;
	int 		nvValue;
	int			nvLength=0;
	int         NumNetVars=0;

	status_struct NeuronStatus;
	//int	retry_count=0;
	nv_struct   nvStructData;

	choice=get_node_choice();

	setup_NID_addrress(&send_addr,NCIPtr);
	

	if (!query_status(&send_addr,&NeuronStatus ))
	{
		fprintf(lon_msg_log,"ERROR on Query status.. exiting.\n");
		return;
	}
    print_neuron_status_struct(&NeuronStatus);

				
//get # of network variables.  If >0 then find infon on them.

	if(get_num_net_vars(NCIPtr,&NumNetVars))
	//read_memory(&send_addr,READ_ONLY_RELATIVE,0x0A,1,&response_data))
	{
		fprintf(lon_msg_log,"Network Variable Count=%d \n",NumNetVars);
				
		for(int nv_index=0; nv_index < NumNetVars; nv_index++)
		{
		
			nvMsgData.exp.data[0]=0;
			nvMsgData.exp.data[1]=0;
			nvMsgData.exp.data[2]=0;
			nvMsgData.exp.data[3]=0;
			nvMsgData.exp.data[4]=0;
			nvMsgData.exp.data[5]=0;
						
			if(nvLength==fetch_nv(&send_addr,nv_index,&nvMsgData))
			{
				nvValue=(int) convert_byte_array_to_int(&nvMsgData.exp.data[1],nvLength);
				
				fprintf(lon_msg_log,"NV # %d = %d = 0x%04X ; LENGTH=%d\n",nvMsgData.exp.data[0],nvValue,nvValue,nvLength);
			}
			if(fetch_nv_cnfg(&send_addr,nv_index,&nvStructData))
			{
				
				if(nvStructData.priority)
					fprintf(lon_msg_log,"nv_priority=YES      ;  ");
				else
					fprintf(lon_msg_log,"nv_priority=NO       ;  ");

				if(nvStructData.direction)
					fprintf(lon_msg_log,"nv_direction=1 (OUTPUT) ");
				else
					fprintf(lon_msg_log,"nv_direction=0 (INPUT)  ");
				
				fprintf(lon_msg_log,"\n");
				
				fprintf(lon_msg_log,"nv_selector_hi = %02x ; nv_selector_lo = %02x  ; combined=%02X%02X\n",nvStructData.selector_hi,nvStructData.selector_lo,nvStructData.selector_hi,nvStructData.selector_lo);

				if(nvStructData.service==0)
					fprintf(lon_msg_log,"Service=ACKD         ; ");
				else if(nvStructData.service==1)
					fprintf(lon_msg_log,"Service=UNACKD_RPT   ; ");
				else if(nvStructData.service==2)
					fprintf(lon_msg_log,"Service=UNACKD       ; ");
				else
					fprintf(lon_msg_log,"Service= (UNKNOWN) ; ");
			
				
				if(nvStructData.auth)
					fprintf(lon_msg_log,"nv_auth=YES  ; ");
				else
					fprintf(lon_msg_log,"nv_auth=NO   ; ");
					
				if(nvStructData.turnaround)
					fprintf(lon_msg_log,"nv_turnaround=YES ; ");
				else
					fprintf(lon_msg_log,"nv_turnaround=NO  ; ");
		
				fprintf(lon_msg_log,"nv_addr_index=%d \n",nvStructData.addr_index);

			}
									
			fprintf(lon_msg_log,"\n");
		}
	}

}
/////////////////////////////////////////////////////////////////////////////////////////////
//  Function:
//
//  Author: Karl Hiramoto
//  
//  Date:  16 Aug 2000
//
//  Description:
//
//
//  Parameters:
//
//  Return Value:
//
//  Modification Log:
//
//
///////////////////////////////////////////////////////////////////////////////////////////

/*
void reset_node_menu(void)
{

    short int choice=0;
    SendAddrDtl send_addr;
    
    choice = get_node_choice();
    
    setup_NID_addrress(&send_addr,&NetworkNodeTable[choice]);
    if (!reset_node(&send_addr))
    {// error
        fprintf(lon_msg_log,"ERROR CALLING RESET_NODE. \n");
    
    }


}
*/

/////////////////////////////////////////////////////////////////////////////////////////////
//  Function:
//
//  Author: Karl Hiramoto
//  
//  Date:  16 Aug 2000
//
//  Description:
//
//
//  Parameters:
//
//  Return Value:
//
//  Modification Log:
//
//
///////////////////////////////////////////////////////////////////////////////////////////
bool recalculate_checksum(SendAddrDtl  *send_addr,NM_checksum_recalc_request_type  kind_of_recalc )
{

	NI_Code      status = NI_OK;
	//SendAddrDtl outAddr;
	MsgData     outData;
	MsgData     response_data;
	int         response_length;
	ComplType   completion;
	int num_responses=0;
	RespAddrDtl response_addr;
	//status_struct *NeuronStatusPtr;
	int retry_count=0;

	

// test
//	fprintf(lon_msg_log,"Sending to neuron ID = ");
//	print_array(send_addr->id.nid,NEURON_ID_LEN,"%02X ");
    fprintf(lon_msg_log,"\n");


	outData.exp.code = NM_checksum_recalculate;
	outData.exp.data[0]= (byte) kind_of_recalc;

	retry_checksum:
	response_data.exp.code=0;

   /* Ship out the message and wait for the response: */
   status = ni_send_msg_wait(REQUEST,
   				 send_addr,  // send to address
				 &outData,  // send data
				  2,        // data length
				  false,	// priority
				  false, // send_auth
                  &completion, // completion code
		          &num_responses,  // number of responses
			      &response_addr,	// resp_addr
			      &response_data,  // resp_data
			      &response_length);  // resp length

	if (status == NI_OK && completion != MSG_SUCCEEDS)
	{
		/* The message failed to complete: */
		status = NI_NO_RESPONSES;
		fprintf(lon_msg_log,"Status=NI_NO_RESPONSES recalc_checksum()\n");
	}
	
	if (response_data.exp.code!= 0x2f && retry_count < MAX_ERROR_RETRY_COUNT)
	{
		retry_count++;
		fprintf(lon_msg_log,"Error on update checksum.. Retrying in 3 seconds");
		sleep(3);
		goto retry_checksum;
	
	}



	if (response_data.exp.code== 0x2f)
	{
	//	fprintf(lon_msg_log,"Checksum Recalc WAS A SUCCESS\n");
	    return true;	
	}
	else
		fprintf(lon_msg_log,"Checksum Recalc WAS A FAILURE\n");
    	fprintf(lon_msg_log,"Response code to Checksum Recalc is = %02X    Num_Responses=%d \n",response_data.exp.code,num_responses);
		
    ( void ) handle_error(status, completion, response_data.exp.code,
        "updating program ID" );           // report any errors

    return false;
}


/////////////////////////////////////////////////////////////////////////////////////////////
//  Function:
//
//  Author: Karl Hiramoto
//  
//  Date:  16 Aug 2000
//
//  Description:
//
//
//  Parameters:
//
//  Return Value:
//
//  Modification Log:
//
//
///////////////////////////////////////////////////////////////////////////////////////////
bool write_memory(SendAddrDtl  *send_addr,byte mode,unsigned short int offset, byte count, byte form, byte *data)
{
	const int MAX_DATA_LENGTH = 11;

	
	NI_Code      status = NI_OK;
	//SendAddrDtl outAddr;
	//MsgData     outData;
	RespAddrDtl response_addr;
	MsgData     response_data;
	int         response_length;
	ComplType   completion;
	int num_responses=0;
	//bool reset_node_flag=false;
	int retry_count=0;
	
    struct {
        NM_write_memory_request     header;
        byte     data[ MAX_DATA_LENGTH ];           // data to be written
    } write_mem_msg;
    
	write_mem_msg.header.code=NM_write_memory;
	write_mem_msg.header.mode=mode; // absolute addressing mode
    write_mem_msg.header.count=count;
    write_mem_msg.header.form=form;
    memcpy(&write_mem_msg.data[0],data,count);
	write_mem_msg.header.offset_hi= (byte) (offset >> 8);
	write_mem_msg.header.offset_lo= (byte) (offset & 0x00FF);


	retry_count=0;
	response_data.exp.code=0; // reset.
	
	retry_write_mem:
		
   /* Ship out the message and wait for the response: */
   status = ni_send_msg_wait(REQUEST,
   				  send_addr,  // send to address
				 (MsgData *) &write_mem_msg,  // send data
				  (sizeof(NM_write_memory_request) +write_mem_msg.header.count),        // data length
				  false,	// priority
				  false, // send_auth
                  &completion, // completion code
		          &num_responses,  // number of responses
			      &response_addr,	// resp_addr
			      &response_data,  // resp_data
		    	  &response_length);  // resp length

   if (status == NI_OK && completion != MSG_SUCCEEDS)
   {
      /* The message failed to complete: */
      status = NI_NO_RESPONSES;
      fprintf(lon_msg_log,"Status=NI_NO_RESPONSES write_memory()\n");
	      
      if (retry_count <= MAX_ERROR_RETRY_COUNT)
      {
			retry_count++;
//			fprintf(lon_msg_log,"RETRY WRITE \n");
			sleep(1);
      		goto retry_write_mem;
      }
      else
      {
      		fprintf(lon_msg_log,"ERROR ON WRITE.  TOO MANY RETRY ATTEMPTS \n");
			return false;
      
      }
      
   }
	if (response_data.exp.code== 0x2e)
	{
		
//			fprintf(lon_msg_log,"Write WAS A SUCCESS\n");
	    return true;	
	}	
		
	else
	{
		fprintf(lon_msg_log,"Write WAS A FAILURE\n");
		fprintf(lon_msg_log,"Response code to write is = %02X    Num_Responses=%d \n",response_data.exp.code,num_responses);

	    ( void ) handle_error( status, completion, response_data.exp.code,
        "downloading program" );           // report any errors

		return false;
	}



	return true;
}


/////////////////////////////////////////////////////////////////////////////////////////////
//  Function:
//
//  Author: Karl Hiramoto
//  
//  Date:  16 Aug 2000
//
//  Description:
//
//
//  Parameters:
//
//  Return Value:
//
//  Modification Log:
//
//
///////////////////////////////////////////////////////////////////////////////////////////

/*
void send_nv_to_node(void)
{

	SendAddrDtl  send_addr;
	unsigned short int choice=0;
	MsgData		nvMsgData;
	MsgData		response_data;

	int 		nvValue;
	int			nvLength=0;
	//int         response_length;
	//ComplType   completion;
	//int num_responses=0;
	//nv_struct   nvStructData;
	int num_network_variables=0;
	byte nvNumber;
	long int NewNVValue=0;
	byte nv_index=0;
	
	fprintf(lon_msg_log,"Which node would you like to write a Network Variable to\n");
	
	choice=get_node_choice();
	setup_NID_addrress(&send_addr,&NetworkNodeTable[choice]);
	

	if(!read_memory(&send_addr,READ_ONLY_RELATIVE,0x0A,1,&response_data))
	{
		fprintf(lon_msg_log,"ERROR: read_memory error\n");
		return;
	  
	}  

								
	num_network_variables=response_data.exp.data[0];  // NUMBER of NV's
	
	nv_index=(byte) get_nv_choice(); // get which NV they want to change.
	
	if (nv_index >= num_network_variables)
	{
		fprintf(lon_msg_log,"ERROR  choice out of range\n");
		return;
	  
	}  
	
	nvMsgData.exp.data[0]=0;
	nvMsgData.exp.data[1]=0;
	nvMsgData.exp.data[2]=0;
	nvMsgData.exp.data[3]=0;
	nvMsgData.exp.data[4]=0;
	nvMsgData.exp.data[5]=0;
						
						
	// get the byte length of the NV and the current value.
	nvLength=fetch_nv(&send_addr,nv_index,&nvMsgData);
	
	// if previous fetch_nv returned an error
	if(nvLength==0)
	{
		fprintf(lon_msg_log,"ERROR  fetch_nv returned error\n");
		return;
	}
	
	nvValue=(int) convert_byte_array_to_int(&nvMsgData.exp.data[1],nvLength);
	nvNumber=nvMsgData.exp.data[0];
				
	fprintf(lon_msg_log,"Currently NV # %d = %d = 0x%04X ; LENGTH=%d (bytes)\n", nvNumber, nvValue, nvValue, nvLength);


	fprintf(lon_msg_log,"What would you like to set NV# %d  to?  --> ",nvNumber);
    
    if(!scanf("%ld", &NewNVValue) )
	{
		fprintf(lon_msg_log,"ERROR: Invalid input \n");
		return;
	}

	fprintf(lon_msg_log,"DEBUG..  You selected  =%ld = 0x%04lX \n", NewNVValue, NewNVValue);


	if(!write_nv(&send_addr,nv_index,nvLength,NewNVValue))
	{
		fprintf(lon_msg_log,"ERROR:  write_nv returned error\n");
		return;
	}

}
*/

/////////////////////////////////////////////////////////////////////////////////////////////
//  Function:
//
//  Author: Karl Hiramoto
//  
//  Date:  16 Aug 2000
//
//  Description:
//
//
//  Parameters:
//
//  Return Value:
//
//  Modification Log:
//
//
///////////////////////////////////////////////////////////////////////////////////////////
bool write_neuron_NV(NeuronCalibrationItem *CTSPtr,byte nv_index,long long nv_data)
{

    int nvLength;
	SendAddrDtl  send_addr;
    MsgData nvMsgData;
    
    if (CTSPtr->Neuron_ID == 0)
        return false;
    
	setup_NID_addrress(&send_addr,CTSPtr);
    
    
	nvMsgData.exp.data[0]=0;
	nvMsgData.exp.data[1]=0;
	nvMsgData.exp.data[2]=0;
						
						
	// get the byte length of the NV and the current value.
	nvLength=fetch_nv(&send_addr,nv_index,&nvMsgData);
	
	// if previous fetch_nv returned an error
	if(nvLength==0)
	{
		fprintf(lon_msg_log,"ERROR  fetch_nv returned error\n");
		return false;
	}
	
	if(!write_nv(&send_addr,nv_index,nvLength,nv_data))
	{
		fprintf(lon_msg_log,"ERROR:  write_nv returned error\n");
		return false;
	}

    return true;

}

/////////////////////////////////////////////////////////////////////////////////////////////
//  Function:
//
//  Author: Karl Hiramoto
//  
//  Date:  16 Aug 2000
//
//  Description:
//
//
//  Parameters:
//
//  Return Value:
//
//  Modification Log:
//
//
///////////////////////////////////////////////////////////////////////////////////////////
int read_neuron_NV(NeuronCalibrationItem *CTSPtr,byte nv_index,long long *nv_data)
{

    int nvLength;
	SendAddrDtl  send_addr;
    MsgData nvMsgData;

    if (CTSPtr->Neuron_ID == 0)
        return false;    
	
	setup_NID_addrress(&send_addr,CTSPtr);
    
	nvMsgData.exp.data[0]=0;
	nvMsgData.exp.data[1]=0;
	nvMsgData.exp.data[2]=0;
	nvMsgData.exp.data[3]=0;
	nvMsgData.exp.data[4]=0;
						
						
	// get the byte length of the NV and the current value.
	nvLength=fetch_nv(&send_addr,nv_index,&nvMsgData);
	
	// if previous fetch_nv returned an error
	if(nvLength==0)
	{
		fprintf(lon_msg_log,"ERROR  fetch_nv returned error\n");
		*nv_data=0;
		return 0;
	}
	
	*nv_data= convert_byte_array_to_int(&nvMsgData.exp.data[1],nvLength);


    return nvLength;

}
bool read_neuron_NV_float(NeuronCalibrationItem* CTSPtr, byte nv_index,float *value)
{
    union {
        float   intel_float;
        byte    neuron_float[ 4 ];
    } swapper;
    long long temp_data;
    
    if(read_neuron_NV(CTSPtr,nv_index,&temp_data))
    {
        memcpy(swapper.neuron_float,&temp_data,4);
        *value=swapper.intel_float;
	    return true;
    }
    else     
        return false;
    

    return true;
}
bool write_neuron_NV_float(NeuronCalibrationItem* CTSPtr, byte  nv_index,float value)
{
    union {
        float   intel_float;
        byte    neuron_float[ 4 ];
    } swapper;
    long long temp_data;

     swapper.intel_float=value;
     memcpy(&temp_data,swapper.neuron_float,4);

    return write_neuron_NV(CTSPtr,nv_index,temp_data);
//    return write_neuron_NV(CTSPtr,nv_index,(long long) swapper.neuron_float);

}


bool set_neuron_appless_unconfig_offline(NeuronCalibrationItem *CTSPtr)
{

	SendAddrDtl  send_addr;
	status_struct NeuronStatus;

    if (CTSPtr->Neuron_ID == 0)
        return false;

    setup_NID_addrress(&send_addr,CTSPtr);

	fprintf(lon_msg_log,"--------Starting program of NEURON %Ld ------------",CTSPtr->Neuron_ID);


// test
//	fprintf(lon_msg_log,"Sending to neuron ID = ");
//	print_array(&send_addr.id.nid,NEURON_ID_LEN,"%02X ");
//    fprintf(lon_msg_log,"\n");

//	query_node();
	clear_status(&send_addr);
	sleep(1);
	
//    if (!reset_node(&send_addr))
//    {// error
//        fprintf(lon_msg_log,"ERROR CALLING RESET_NODE. \n");
//    }
    
   	sleep(1);
    if (!query_status(&send_addr,&NeuronStatus))	
    {
        fprintf(lon_msg_log,"ERROR on Query status \a\a\a\n");    
    }
    else
        print_neuron_status_struct(&NeuronStatus);

    
    if (NeuronStatus.node_state & 0x03)
    {
        return true;
    }
    
    sleep(1);
    
    if (NeuronStatus.node_state == 0x04)  // node is configured and online.
    {
    
    	fprintf(lon_msg_log,"seting node offline \n");
    	change_node_mode_state(&send_addr,APPL_OFFLINE,0);
        sleep(1);
    	fprintf(lon_msg_log,"CLEARING STATUS\n");
    	clear_status(&send_addr);
    	sleep(1);
	
	}
	
    if (!query_status(&send_addr,&NeuronStatus))	
    {
        fprintf(lon_msg_log,"ERROR on Query status\n");    
    }
    else
        print_neuron_status_struct(&NeuronStatus);
    
    
	if(NeuronStatus.node_state==0x02) // Unconfigured (but with an application.)
	{
    	fprintf(lon_msg_log,"setting node  applicationless\n");
    	change_node_mode_state(&send_addr,CHANGE_STATE,NO_APPL_UNCNFG);
	    sleep(1);
    	fprintf(lon_msg_log,"CLEARING STATUS\n");
    	clear_status(&send_addr);
    	sleep(1);
	}
	
	
    if (!query_status(&send_addr,&NeuronStatus))	
    {
        fprintf(lon_msg_log,"ERROR on Query status\n");    
    }
    else
        print_neuron_status_struct(&NeuronStatus);
	

	if(NeuronStatus.node_state==0x8C) //Bypass offline
	{
	    fprintf(lon_msg_log,"Node was in Bypass ofline State.  Resetting  \n");
        if (!reset_node(&send_addr))
            fprintf(lon_msg_log,"ERROR CALLING RESET_NODE. \n");
	    
		fprintf(lon_msg_log,"CLEARING STATUS");
    	clear_status(&send_addr);
    	sleep(1);
	}

	if(NeuronStatus.node_state==0x0C) // Unconfigured (but with an application.)
	{
    	fprintf(lon_msg_log,"CLEARING STATUS");
    	clear_status(&send_addr);
    	sleep(1);

    	fprintf(lon_msg_log,"setting node  applicationless NO_APPL_UNCNFG \n");
    	change_node_mode_state(&send_addr,CHANGE_STATE,NO_APPL_UNCNFG);

	    sleep(1);
	}

   	fprintf(lon_msg_log,"CLEARING STATUS\n");
   	clear_status(&send_addr);
   	sleep(1);
	
	
	if (!query_status(&send_addr,&NeuronStatus))	
    {
        fprintf(lon_msg_log,"ERROR on Query status\n");    
    }
    else
        print_neuron_status_struct(&NeuronStatus);
	
	
			
	
	if( !(NeuronStatus.node_state & 0x03)) // APPlication less and Unconfig   // Maybe check bits
	{
	    fprintf(lon_msg_log,"WE CAN NOT program the Node.  state needs to be 0x03 or bits = to that \a\a\a \n");
	    return false;
	}

    

    return true;
}


/////////////////////////////////////////////////////////////////////////////////////////////
//  Function: download_to_neuron
//
//  Author: Karl Hiramoto
//  
//  Date:  16 Aug 2000
//
//  Description:   this func takes a hex file line from the NXE file and breaks it into
//                 packets of max data size = MAX_DATA_LENGTH.   then calles write_memory to send the
//                 correct data
//
//
//  Parameters:
//
//  Return Value:    true on success. false on error.
//
//  Modification Log:
//
//
///////////////////////////////////////////////////////////////////////////////////////////
bool download_to_neuron(SendAddrDtl  *send_addr,hex_file_line_type  *neuron_program)
{
	const int MAX_DATA_LENGTH = 11;

	byte data_remaining_to_write=0;
	//bool reset_node_flag=false;
	//int retry_count=0;
    word write_offset=0;
    byte write_byte_count;		

	data_remaining_to_write=neuron_program->record_length; // length to write;

	
	while(data_remaining_to_write)
	{

        write_offset= neuron_program->address;
	
		if (data_remaining_to_write >  MAX_DATA_LENGTH) // still will have another packet after this one.
            write_byte_count=MAX_DATA_LENGTH;
		else
			write_byte_count=data_remaining_to_write; // we can fit it into one write.

        // send the data
        if(!write_memory(send_addr,ABSOLUTE,neuron_program->address,write_byte_count,NO_ACTION,
	        &neuron_program->data[neuron_program->record_length - data_remaining_to_write]))
	        return false; // return error
		
        // still will have another packet after this one.
   		if (data_remaining_to_write >  MAX_DATA_LENGTH)
		{
    		data_remaining_to_write-=MAX_DATA_LENGTH; // compute remaining data to write later
   			neuron_program->address+=MAX_DATA_LENGTH; // update the address of the next write to perform
			
        }
		else // this was the last packet to write.
			data_remaining_to_write=0;  // so we may exit the while loop.
			
		usleep(100000); // sleep for   .100 seconds.  the neuron can't handle messages coming in too fast.

	}


	return true;
}



bool download_nxe_code(NeuronCalibrationItem *CTSPtr, const char * NXEFile)
{
	byte line[80];
	int i,count=0;
	char temp_str[10]={0,0,0,0,0,0,0,0,0,0};
	FILE *FilePtr;
	hex_file_line_type first_data_line;  // storage for the first line if needed
	hex_file_line_type data_line;  // store every line as we read it.
	bool first_line=true; // flag if we are reading the first line of the data file
	bool delay_first_line_write=false;  // if the first line has a length of 1 delay it till the nend.
	bool node_needs_reset=true;  // if node was not reset during programming then reset laterl
	status_struct NeuronStatus;


	SendAddrDtl  send_addr;

    if (CTSPtr->Neuron_ID == 0)
        return false;

    setup_NID_addrress(&send_addr,CTSPtr);

    
	//	sleep(1);
    if (!query_status(&send_addr,&NeuronStatus))	
    {
        fprintf(lon_msg_log,"ERROR on Query status\n");    
    }
    else
        print_neuron_status_struct(&NeuronStatus);

	    
    
   	fprintf(lon_msg_log,"------------Starting download.-----------------\n");

   	fprintf(lon_msg_log,"file =%s\n",NXEFile);

	fprintf(lon_msg_log,"CLEARING STATUS\n");
	clear_status(&send_addr);


	FilePtr=fopen(NXEFile,"r");
	if (FilePtr == NULL) // error on file open.
	{
		fprintf(lon_msg_log,"ERROR on file open\n");
		return false;
	}
	
	for (i=0; i < 80 ; i++) // initialize line storage container.
		line[i]=0; // initialize to all null.
		
	
// read through the file  line by line.   
    fprintf(lon_msg_log,"\n\nDownloading program: ");
    
//    return;
    
	while (fgets((char*)&line[0],80,FilePtr))
	{

		if(line[0] != ':')
			fprintf(lon_msg_log,"ERROR READING FILE char-->%c<--",line[0]);
		else fprintf(lon_msg_log,"Line Read = %s",line);
			
		data_line.record_marker=':'; // ':'

        // get the record length byte
	 	strncpy(&temp_str[0],(char*)&line[1],2); // copy next two bytes of line into temp_str.  pad with null's
		temp_str[2]=0; //null terminate
		data_line.record_length=(byte) strtoul(&temp_str[0],NULL,16); // convert acsii base 16 number to binary
	

    	// get the address  word
		strncpy(&temp_str[0],(char*)&line[3],4); // copy four bytes into temp_str
		data_line.address = (word) strtoul(&temp_str[0],NULL,16);
		
		// get the record type  byte
		strncpy(&temp_str[0],(char*)&line[7],2); // copy two bytes into temp_str
		temp_str[2]=0; //null terminate
		data_line.record_type= (hex_record_type) strtoul(&temp_str[0],NULL,16);

		temp_str[2]=0; //null terminate

		// read in data values
		for(count=0, i=9 ; i < 9+(2 * data_line.record_length) ; i=i+2, ++count)
		{ 
			strncpy(&temp_str[0],(char*) &line[i],2);  // copy two bytes from line into temporary string.
			data_line.data[count]=(byte) strtoul(&temp_str[0],NULL,16);  //convert ascii HEX valuse to binary
		}
		
		strncpy(&temp_str[0],(char *) &line[9+(2 * data_line.record_length)],2); // copy two bytes into temp_str
		data_line.checksum= (hex_record_type) strtoul(&temp_str[0],NULL,16);

		fprintf(lon_msg_log,"data length=%d ;address =0x%04X ;TYPE=%d ;CheckSum= 0x%02X ; Data Values (hex):\n",data_line.record_length,data_line.address,data_line.record_type,data_line.checksum);
//		print_array(&data_line.data,data_line.record_length,"%02X ");
//		fprintf(lon_msg_log,"\n");
        fprintf(lon_msg_log,".");
				
		if (first_line && (data_line.record_length==1))
		{
			memcpy(&first_data_line,&data_line,sizeof(data_line));
			first_line=false;
			delay_first_line_write=true;
		}
		else
		{  //write stuff to the nuron
		
		    
		    if (data_line.record_length==0)
		    {
		        fprintf(lon_msg_log,"\nIn program node. record_length = 0 ;  Resetting node\n");
                if (!reset_node(&send_addr))
                    fprintf(lon_msg_log,"ERROR CALLING RESET_NODE. \n");
		        else
    			    node_needs_reset=false; // node was reset.  no need to do it later.
		    
		    }
		    else if(!download_to_neuron(&send_addr,&data_line))
    			{
	    			fprintf(lon_msg_log,"ERROR on download to neuron.  maybe try again. BYPASS ERROR\n");
		    		sleep(2);
    			//	return;  report the error but don't exit
	    		}
		
		}
		
		for (i=0; i < 80 ; i++) // initialize line storage container.
			line[i]=0; // initialize to all null.
	
		first_line=false;	// by this point we are no longer reading the first line.
	} // end of while loop to read data file lines.
	
	fclose(FilePtr);  // close file.  We are done reading it.

//	fprintf(lon_msg_log,"\n\n");
	
	clear_status(&send_addr);
	
		
	if (delay_first_line_write)
	{  // write it now.
    	fprintf(lon_msg_log,"NOW WRITING FIRST LINE) \n");
    	sleep(1);
	    if (!download_to_neuron(&send_addr,&first_data_line))
		{
			fprintf(lon_msg_log,"ERROR on download to neuron.  maybe try again. BYPASSING ERROR (FIRST LINE) \n");
			sleep(1);
			// return; report the error but don't exit
		}
	
	}

    sleep(1);
    
	// reset here if necessary.
	if (node_needs_reset)
	{
	
		fprintf(lon_msg_log,"Reseting node. in program_node()\n");

	    if(!reset_node(&send_addr))
	    {
	        fprintf(lon_msg_log,"Error reseting node\n");
	    }
        else
	    {
	        sleep(3);
		    if (!query_status(&send_addr,&NeuronStatus))	
                fprintf(lon_msg_log,"ERROR on Query status\n");    
            else
                print_neuron_status_struct(&NeuronStatus);

        }

	
	}
	
			
// to debug
//report_flag  = true;     // report incoming application msgs
//verbose_flag = true;    // display all network interface msgs
//online_flag  = true;     // This node is on-line

			
			
// compute checksum here.
    fprintf(lon_msg_log,"Recaclulating checksum \n");
    if(recalculate_checksum(&send_addr,BOTH_CS))
    {
    
        fprintf(lon_msg_log,"Success Recalc Checksum\n");
    }
    else
        fprintf(lon_msg_log,"Fail recalc Checksum\n");

//    sleep(3);

    return true;
}



bool set_node_online(NeuronCalibrationItem *CTSPtr)
{

	SendAddrDtl  send_addr;
	status_struct NeuronStatus;
    int retry_count=0;

    if (CTSPtr->Neuron_ID == 0)
        return false;

    setup_NID_addrress(&send_addr,CTSPtr);

	fprintf(lon_msg_log,"--------Seting node online/configured NEURON %Ld ------------",CTSPtr->Neuron_ID);




	fprintf(lon_msg_log,"setting state  configured online. two to go\n");
    if(!change_node_mode_state(&send_addr,CHANGE_STATE,CNFG_ONLINE))
    {
        fprintf(lon_msg_log,"Error setinging node configured and online. \n");
    }

    sleep(1);
	if (!query_status(&send_addr,&NeuronStatus))	
        fprintf(lon_msg_log,"ERROR on Query status\n");    
    else
        print_neuron_status_struct(&NeuronStatus);



	if( NeuronStatus.node_state != 0x04) // error
	{
    	fprintf(lon_msg_log,"Error setting node online");
//	    return;
	}
	
    sleep(1);
    
    
    
   	fprintf(lon_msg_log,"setting mode  offline. one to go\n");
    if(!change_node_mode_state(&send_addr,APPL_OFFLINE,0))
    {
        fprintf(lon_msg_log,"Error setinging node offline. \n");
    }

    sleep(1);
	if (!query_status(&send_addr,&NeuronStatus))	
        fprintf(lon_msg_log,"ERROR on Query status\n");    
    else
        print_neuron_status_struct(&NeuronStatus);


    retry_count=0;
    retry_final_online:
    
	fprintf(lon_msg_log,"setting mode online FINAL \n");
    if (!change_node_mode_state(&send_addr,APPL_ONLINE,0))  // ackd msg.
    {
        fprintf(lon_msg_log,"Error setinging node online. \n");
    }
    sleep(1);
    
    if (!query_status(&send_addr,&NeuronStatus))	
        fprintf(lon_msg_log,"ERROR on Query status\n");    
    else
        print_neuron_status_struct(&NeuronStatus);

  	if( NeuronStatus.node_state != 0x04) // ONLINE
	{
	    fprintf(lon_msg_log,"Node did not come online.  Retry\n");
   	    retry_count++;
	    if (retry_count <  MAX_ERROR_RETRY_COUNT)
	        goto retry_final_online;
		else
    	    fprintf(lon_msg_log,"Node did not come online. ERROR \n");
		 
		return false;
	}

    return true;
}


bool set_location_string(NeuronCalibrationItem *CTSPtr,string Location)

{
	SendAddrDtl  send_addr;
    byte *DataPtr = new byte[6];
    
    DataPtr[0]=0;
    DataPtr[1]=0;
    DataPtr[2]=0;
    DataPtr[3]=0;
    DataPtr[4]=0;
    DataPtr[5]=0;

    
    if (CTSPtr->Neuron_ID == 0)
        return false;

    setup_NID_addrress(&send_addr,CTSPtr);
	
	    
    write_memory(&send_addr,             // address of node to write to.
                    CONFIG_RELATIVE,     // write offset is config relative
		            0x02,                // offset
			        0x06,                // length =6
				    CNFG_CS_RECALC,      // update the configureation checksum
				    DataPtr);            // data to write

    delete DataPtr;
    
    return true;
}

// set the channel ID in the config struct
// has no effect on program.  only helps LonBuilder document network 
bool set_channel_id(NeuronCalibrationItem *CTSPtr,short int Channel)
{
	SendAddrDtl  send_addr;
    byte *DataPtr = new byte[2];
    
    if (CTSPtr->Neuron_ID == 0)
       return false;

    setup_NID_addrress(&send_addr,CTSPtr);
	

    DataPtr[0]=Channel & 0x00FF;
    DataPtr[1]=(Channel & 0xFF00) >> 8;
    write_memory(&send_addr,CONFIG_RELATIVE,0x00,0x02,CNFG_CS_RECALC,DataPtr);
    delete DataPtr;
    
    
    return true;

}

// program the common stuff
// about every neuron has this same stuff
bool program_neuron_common_features(NeuronCalibrationItem *CTSPtr,const char * ImageFile,byte subnet, byte node)
{

	//const char record_marker = ':';
	SendAddrDtl  send_addr;
	//unsigned short int choice=0;
	//byte next_char=0;
    status_struct NeuronStatus;

 
    if (CTSPtr->Neuron_ID == 0)
       return false;

    setup_NID_addrress(&send_addr,CTSPtr);
    
				
    if(!set_neuron_appless_unconfig_offline(CTSPtr))
        return false;
				
								
//	sleep(1);

    if(!download_nxe_code(CTSPtr,ImageFile)) // download the nxe file
        return false;


    fprintf(lon_msg_log,"leave Domain\n");
    leave_domain(&send_addr);
    
    fprintf(lon_msg_log,"Clear Address\n");
    clear_address_table(&send_addr);

    fprintf(lon_msg_log,"Init Domain\n");
    init_domain(&send_addr,subnet,node);

    // write data to the config space/
    //   write   0b 01010111
    //  0101 to the non group timer  Table A.1 In motorola databook
    //  0 to nm_Auth  auth will not be used for us
    //  111 to  preemtion_timeout  max timeout is 14 seconds.


    fprintf(lon_msg_log,"WRITING To config space\n");

    byte *DataPtr = new byte[1];
    DataPtr[0]=0x57; // 87 decimal  set non_group_timer_and preemtion_timer
    write_memory(&send_addr,CONFIG_RELATIVE,0x18,1,CNFG_CS_RECALC,DataPtr);
    delete DataPtr;


    sleep(2);
    if (!query_status(&send_addr,&NeuronStatus))	
         fprintf(lon_msg_log,"ERROR on Query status\n");    
    else
         print_neuron_status_struct(&NeuronStatus);


    int retry_count=0;
    retry_set_node_offline:
    
	fprintf(lon_msg_log,"setting node  configured ofline\n");
	change_node_mode_state(&send_addr,CHANGE_STATE,CNFG_OFFLINE);

    sleep(2);
    if (!query_status(&send_addr,&NeuronStatus))	
         fprintf(lon_msg_log,"ERROR on Query status\n");    
    else
         print_neuron_status_struct(&NeuronStatus);


  	if( NeuronStatus.node_state != 0x06) // ONLINE
	{
	    fprintf(lon_msg_log,"Node did not come online.  Retry\n");
	    retry_count++;
	    if (retry_count <  MAX_ERROR_RETRY_COUNT)
	        goto retry_set_node_offline;
    }

    fprintf(lon_msg_log,"WRITING To config space 2\n");

    set_location_string(CTSPtr,"      ");

    fprintf(lon_msg_log,"WRITING To config space 3\n");

    set_channel_id(CTSPtr,0);

    return true;    
}



/////////////////////////////////////////////////////////////////////////////////////////////
//  Function:program_NEIFile_to_neuron
//
//  Author: Karl Hiramoto
//  
//  Date:  16 Oct 2001
//
//  Description:  Downloads the NEI file to the neuron chip.  Use LonBuilder to generate the NEI file.  Then you must
//                also specify the same subnet and node number as lonBuilder uses if you have any network variables bound.
//
//
//  Parameters:
//
//  Return Value:  true on success.  False on Error.
//
//  Modification Log:
//
//
///////////////////////////////////////////////////////////////////////////////////////////
bool program_NEIFile_to_neuron(NeuronCalibrationItem *CTSPtr,const char * NEIFile,int subnet, int node)
{

	SendAddrDtl  send_addr;
//	status_struct NeuronStatus;

     
    if (CTSPtr->Neuron_ID == 0)
       return false;

    setup_NID_addrress(&send_addr,CTSPtr);


    program_neuron_common_features(CTSPtr,NEIFile,subnet,node);
	    
    sleep(2);
/*    
    if (!query_status(&send_addr,&NeuronStatus))	
         fprintf(lon_msg_log,"ERROR on Query status\n");    
    else
         print_neuron_status_struct(&NeuronStatus);


   setup_NuAire_nv_config(&send_addr);

*/

    // bring the node up.    
	set_node_online(CTSPtr);
	
	return true;
	
}


bool  Reset_Neuron(NeuronCalibrationItem *CTSPtr)
{

	SendAddrDtl  send_addr;
//	status_struct NeuronStatus;

     
    if (CTSPtr->Neuron_ID == 0)
       return false;

    setup_NID_addrress(&send_addr,CTSPtr);

    return reset_node(&send_addr);
}


//return a NeuronItem
void get_node_service_msg(NeuronCalibrationItem *NewNeuron)
{
	NI_Code         ni_error;
	ServiceType     service;        /* ACKD, UNACKD_RPT, UNACKD, REQUEST */
	RcvAddrDtl      in_addr;        /* address of incoming msg */
	bool			in_auth;        /* if incoming was authenticated */
	MsgData         in_data;        /* data of incoming msg */
	int             in_length;      
    NM_service_pin_msg  Service_Pin_Data;

 		in_data.exp.code=0; // init
    
	    ni_error = ni_receive_msg(     // Check for network activity
    	        & service,        /* handle incoming message to this node */
        	    & in_addr,
            	& in_data,
	            & in_length,
    	        & in_auth );
		
		if (ni_error == NI_OK)
			if(in_data.exp.code == NM_service_pin_message_code)  // was it a service pin message?
			{
			    memcpy(&Service_Pin_Data,&in_data.exp.data,sizeof(Service_Pin_Data));
		
		    	NewNeuron->Neuron_ID=convert_byte_array_to_int(&Service_Pin_Data.neuron_id,sizeof(Service_Pin_Data.neuron_id));	
            	NewNeuron->Program_ID=convert_byte_array_to_int(&Service_Pin_Data.program_id,sizeof(Service_Pin_Data.program_id));
                NewNeuron->SetNetworkState(ON_NETWORK);
		    }
		    else
		    {
		    	NewNeuron->Neuron_ID=0;
            	NewNeuron->Program_ID=0;
		    }
	    else
	    {
	    	NewNeuron->Neuron_ID=0;
           	NewNeuron->Program_ID=0;
	    }		    
		    
		    // else NeuronID will be 0


}

/*************************************************************
 *
 *            Command to Add a Node
 *
 *************************************************************/
/*
void add_node( void )
{
	bool ServicePinReceived=false;
	char   KeyPressed;
	NI_Code         ni_error;
	ServiceType     service;        // ACKD, UNACKD_RPT, UNACKD, REQUEST 
	RcvAddrDtl      in_addr;        /// address of incoming msg 
	bool			in_auth;        // if incoming was authenticated 
	MsgData         in_data;        // data of incoming msg 
	int             in_length;      
    NM_service_pin_msg  Service_Pin_Data;

	fprintf(lon_msg_log,"Waiting for service Pin (Press escape to abort)\n");

	while (ServicePinReceived==false)
	{

		if( (KeyPressed = getch( )) != ERR )       // Check for keyboard activity
		{
			KeyPressed=getch();
			if (KeyPressed==27) // ESCAPE
			{
				fprintf(lon_msg_log,"Escape pressed.  Aborting service pin wait \n");  
				return;
			}
    	}
 
	    ni_error = ni_receive_msg(     // Check for network activity
    	        & service,        // handle incoming message to this node 
        	    & in_addr,
            	& in_data,
	            & in_length,
    	        & in_auth );
		
		if (ni_error == NI_OK)
			if(in_data.exp.code == NM_service_pin_message_code)  // was it a service pin message?
			{
			    fprintf(lon_msg_log,"\nService Pin Message Recived\n");
			    ServicePinReceived=true;
//				Service_Pin_Data=&in_data.exp.data;
			    memcpy(&Service_Pin_Data,&in_data.exp.data,sizeof(Service_Pin_Data));

			    fprintf(lon_msg_log,"Neuron id= ");
			    print_array(&Service_Pin_Data.neuron_id,NEURON_ID_LEN,"%02X ");
  			    fprintf(lon_msg_log,"\n");

			    fprintf(lon_msg_log,"Program id= ");
			    print_array(&Service_Pin_Data.program_id,sizeof(Service_Pin_Data.program_id),"%02X ");
   			    fprintf(lon_msg_log,"\n");
			    
			    fprintf(lon_msg_log,"Address= ");
     		    print_array(&in_addr,sizeof(in_addr),"%02X ");
   			    fprintf(lon_msg_log,"\n");

				print_address_info(in_addr);
						
			}
	
	}

//	init_calibration_table_struct(&NetworkNodeTable[NumNetworkNodes]);
  
    fprintf(lon_msg_log,"Adding Node. NodeID= %d \n",NumNetworkNodes);

    
	NetworkNodeTable[NumNetworkNodes].Neuron_ID=convert_byte_array_to_int(&Service_Pin_Data.neuron_id,sizeof(Service_Pin_Data.neuron_id));	
	NetworkNodeTable[NumNetworkNodes].Program_ID=convert_byte_array_to_int(&Service_Pin_Data.program_id,sizeof(Service_Pin_Data.program_id));
	NetworkNodeTable[NumNetworkNodes].SetNetworkState(ON_NETWORK);

	NumNetworkNodes++;


	print_all_nodes();
	NetworkNodeTable[NumNetworkNodes].Store(); // karl's test.


}


NeuronCalibrationItem* install_neuron(void)
{
    add_node();
    
    
    // insert struct into DB.
    NetworkNodeTable[NumNetworkNodes-1].Store();
    
    
    
    return (&NetworkNodeTable[NumNetworkNodes-1]);


}
*/

/*************************************************************
 *
 *            Command to report incoming traffic statistics
 *
 *************************************************************/

void traffic( void ) {

    fprintf(lon_msg_log, "Appl msgs received \t= %u\n", num_messages );
    fprintf(lon_msg_log, "Msg data received \t= %lu bytes\n", data_length );
    fprintf(lon_msg_log, "NV updates received \t= %u\n", num_updates );
    fprintf(lon_msg_log, "NV polls received \t= %u\n", num_polls );
    fprintf(lon_msg_log, "Clearing message and NV accumulators\n" );

    data_length = num_messages = num_polls = num_updates = 0;
}

/*
 ****************************************************************************
 *
 * exit_func().  Process exit command from the keyboard, store NV config
 *
 ****************************************************************************
 */

void exit_func( void ) {

    int fd;
    unsigned byte_count;

    fd = creat( "NVCONFIG.TBL", 0644 );
    if( fd < 0 ) {
        fprintf(lon_msg_log, "Unable to open network variable config file\n" );
        exit( 1 );
    }
    byte_count = write( fd, nv_config_table, sizeof( nv_config_table ) );
    if( byte_count != sizeof( nv_config_table ) ) {
        fprintf(lon_msg_log, "Unable to write to network variable config file\n" );
        close( fd );
        exit( 1 );
    }
    close( fd );
    ni_close( );
    fprintf(lon_msg_log,"Leaving the Host Application program.\n");
    exit( 0 );
}

/*************************************************************
 *
 *          get_nv_index - prompt user to enter NV index
 *
 *************************************************************/

int get_nv_index( bool in_only ) {   // returns -1 if invalid

    static const char * dir_string[ 2 ] = { "IN ", "OUT" };
    int nv_index;
    network_variable * nv_value_ptr;

    nv_value_ptr = nv_value_table;
        // list all the network variables - menu label is index+1

    for( nv_index = 0; nv_index < NUM_NVS; nv_index++, nv_value_ptr++ ) {
        if( in_only && nv_value_ptr->direction != NV_IN ) continue;
                             // don't list if not asked for
        fprintf(lon_msg_log, "%d. \t%s\t%s\t", nv_index + 1,
            dir_string[ nv_value_ptr->direction ], nv_value_ptr->name );
        nv_value_ptr->print_func( nv_value_ptr->data );       // print value
    }

	addstr( "Enter NV number :" );
    nv_index = scanw( "%d", nv_index );
    nv_index -= 1;
    
    // If user enters 0 (the default), return error

    if( nv_index >= NUM_NVS || nv_index < 0 ) {
        fprintf(lon_msg_log, "Invalid NV number\n" );
        return -1;
    }

    nv_value_ptr = &nv_value_table[ nv_index ];
    if( in_only && nv_value_ptr->direction != NV_IN ) {
        fprintf(lon_msg_log, "Invalid direction\n" );
        return -1;
    }
    return nv_index;
}

/*************************************************************
 *
 *          Command to poll an input network variable
 *
 *************************************************************/

void NV_poll( void ) {

    int                 nv_index, ta_nv_index;
    NI_Code             ni_error;
    ComplType           completion;
    int                 num_responses;
    int                 in_length;
    SendAddrDtl         out_addr;
    MsgData             in_data, out_data;
    nv_struct           * nv_cnfg_ptr;
    network_variable    * nv_value_ptr;

    fprintf(lon_msg_log, "Which input network variable do you wish to poll?\n" );
    nv_index = get_nv_index ( true );      // get index of NV from user
    if( nv_index < 0 || ! online_flag ) return;
                // all done if NV not found, or node is off-line
    nv_cnfg_ptr = &nv_config_table[ nv_index ];   // point to cnfg table

    // make an outgoing request message to poll the NV
    out_data.unv.direction = NV_OUT;        // to match the receiver(s)
    out_data.unv.NV_selector_hi = nv_cnfg_ptr->selector_hi;
    out_data.unv.NV_selector_lo = nv_cnfg_ptr->selector_lo;
    out_data.unv.must_be_one = 1;

    if( nv_cnfg_ptr->addr_index != NULL_IDX ) {     // NV is bound

        out_addr.im.type = IMPLICIT;
        out_addr.im.msg_tag = nv_cnfg_ptr->addr_index;
            // For implicit addressing, use configured address table entry

        ni_error = ni_send_msg_wait(
            REQUEST,
            & out_addr,             // destination address
            & out_data,
            2,                      // size of selector
            nv_cnfg_ptr->priority,
            nv_cnfg_ptr->auth,
            & completion,
            & num_responses,
            NULL,                   // response address
            & in_data,              // first response data
            & in_length );            // input length

        if( handle_error( ni_error, completion, NO_CHECK,
                "polling NV" ) ) return;

        if( num_responses ) for( ;; ) {       // handle responses, if any

            handle_netvar_poll_response( & in_data.unv, in_length,
                nv_index );
            // update the value if a valid response

            if( !--num_responses ) break;

            ni_error = ni_get_next_response(      // get another, if a group
                NULL,            // response address
                & in_data,       // response data
                & in_length );
            if( handle_error( ni_error, completion, NO_CHECK,
                "polling NV" ) ) return;
        }
    }
    if( !nv_cnfg_ptr->turnaround ) return;
            // all done if not a turnaround

    // get the value of the matching output
    ta_nv_index = match_nv_cnfg( &out_data.unv );
    if( ta_nv_index < 0 ) return;       // no match

    nv_value_ptr = &nv_value_table[ ta_nv_index ]; // point to NV value table
    update_nv_value( nv_index, nv_value_ptr->data, nv_value_ptr->size );
}

/*************************************************************
 *
 *          Command to update a network variable
 *
 *************************************************************/

void NV_update( void ) {

    network_variable    * nv_value_ptr;
    nv_struct           * nv_cnfg_ptr;
    int                 nv_index;
    int                 out_length;
    NI_Code             ni_error;
    ComplType           completion;
    SendAddrDtl         out_addr;
    MsgData             out_data;

    fprintf(lon_msg_log, "Which network variable do you wish to update?\n" );
    nv_index = get_nv_index ( false );    // get NV index from user
    if( nv_index < 0 ) return;

    nv_value_ptr = &nv_value_table[ nv_index ];    // point to value table
    fprintf(lon_msg_log, "Enter data for %s :", nv_value_ptr->name );
    nv_value_ptr->read_func( nv_value_ptr->data );  // get NV data from user

    if( nv_value_ptr->direction == NV_IN || ! online_flag ) return;
            // all done if an input or node is not on-line

    // make an outgoing NV update message

    nv_cnfg_ptr = &nv_config_table[ nv_index ]; // point to config table

    out_data.unv.direction = NV_IN;     // to match the receiver
    out_data.unv.NV_selector_hi = nv_cnfg_ptr->selector_hi;
    out_data.unv.NV_selector_lo = nv_cnfg_ptr->selector_lo;
    out_data.unv.must_be_one = 1;

    out_length = nv_value_ptr->size;
    memcpy( out_data.unv.data, nv_value_ptr->data, out_length );
    out_length += 2;        // add selector size to send

    if( nv_cnfg_ptr->addr_index != NULL_IDX ) {     // NV is bound

        out_addr.im.type = IMPLICIT;        // use the address table for dest
        out_addr.im.msg_tag = nv_cnfg_ptr->addr_index;

        ni_error = ni_send_msg_wait(
            (ServiceType) nv_cnfg_ptr->service,   // use configured service type
            & out_addr,             // destination address
            & out_data,
            out_length,             // including selector
            nv_cnfg_ptr->priority,
            nv_cnfg_ptr->auth,
            & completion,
            NULL,                   // num_responses
            NULL,                   // response address
            NULL,                   // response data
            NULL );                 // input length
        ( void )handle_error( ni_error, completion, NO_CHECK,
                "updating NV" );
    }
    if( !nv_cnfg_ptr->turnaround ) return; // all done if not a turnaround
     // find the matching input NV
    ( void ) handle_netvar_msg( & out_data, UNACKD, out_length, true );
}


/*************************************************************
 *
 *         User interface routines for SNVTs
 *
 *************************************************************/

void print_asc( byte * data ) {     // print a value of type SNVT_str_asc
    fprintf(lon_msg_log, "\"%s\"\n", data );
}

void print_cont( byte * data ) {    // print a value of type SNVT_lev_cont
    fprintf(lon_msg_log, "%g percent\n", * data / 2.0 );
}

void print_disc( byte * data ) {    // print a value of type SNVT_lev_disc

    switch ( * data ) {
        case 0: fprintf(lon_msg_log, "OFF\n" );    return;
        case 1: fprintf(lon_msg_log, "LOW\n" );    return;
        case 2: fprintf(lon_msg_log, "MEDIUM\n" ); return;
        case 3: fprintf(lon_msg_log, "HIGH\n" );   return;
        case 4: fprintf(lon_msg_log, "ON\n" );     return;
        default: fprintf(lon_msg_log, "Invalid discrete value %d\n", * data );
    }
}

void print_float( byte * data ) {   // print a value of type SNVT_xxx_f
    union {
        float   intel_float;
        byte    neuron_float[ 4 ];
    } swapper;
    int   i;

    for( i = 3; i >= 0; i-- )
        swapper.neuron_float[ i ] = * data++;       // reverse byte order
    fprintf(lon_msg_log, "%.5G\n", swapper.intel_float );
}

void read_asc( byte * data ) {      // read a value of type SNVT_str_asc
    char input_buf[ 80 ];

   // gets( input_buf );
   fgets(input_buf,79,stdin);  
     memset( data, 0, 31 );                       // null it out
    strncpy( ( char * )data, input_buf, 31 );    // copy up to 31 bytes
}

void read_disc( byte * data ) {    // read a value of type SNVT_lev_disc
    char ch;
    for( ;; ) {
        fprintf(lon_msg_log, "\nO(F)F, (L)OW, (M)EDIUM, (H)IGH or O(N) :" );
        ch = getch( );
        switch( toupper( ch ) ) {
            case 'F':  * data = 0; break;
            case 'L':  * data = 1; break;
            case 'M':  * data = 2; break;
            case 'H':  * data = 3; break;
            case 'N':  * data = 4; break;
            default : fprintf(lon_msg_log, " -- invalid choice" ); continue;
        }
        fprintf(lon_msg_log, "\n" );
        return;
    }
}

void read_cont( byte * data )      // read a value of type SNVT_lev_cont
{
	fprintf(lon_msg_log, "(percent) " );
    scanw( "%d", * data );
}

void read_float( byte * data ) {   // read a value of type SNVT_xxx_f
    union {
        float   intel_float;
        byte    neuron_float[ 4 ];
    } swapper;
    int   i;
    char  input_buf[ 80 ];

    for( ;; ) {
//        gets( input_buf );
	   fgets(input_buf,79,stdin);  

        if( sscanf( input_buf, "%g", &swapper.intel_float ) == 1 ) break;
        fprintf(lon_msg_log, "Invalid floating point data\n" );
    }

    for( i = 3; i >= 0; i-- )
        * data ++ = swapper.neuron_float[ i ];       // reverse byte order
}

#endif
