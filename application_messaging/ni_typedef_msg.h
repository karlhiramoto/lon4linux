/* network interface typdefs and managment codes for application messaging. */


#ifndef _NI_MGMT_H
#define _NI_MGMT_H	1

#define ID_STR_LEN 8            /* program ID length */

#define NULL_IDX  15            /* unused address table index */

// network variable direction bit.
#define   NV_INPUT 0  
#define   NV_OUTPUT 1

typedef struct {
    bits    selector_hi : 6;
    bits    direction   : 1;
    bits    priority    : 1;
    bits    selector_lo : 8;
    bits    addr_index  : 4;
    bits    auth        : 1;
    bits    service     : 2;
    bits    turnaround  : 1;
} nv_struct;

// Note - Microsoft C will make nv_struct 4 bytes long because it does
// not allow odd-length bit-fields.  It is really 3 bytes long.
// Be careful when using sizeof() any structure that includes an nv_struct.

#define NM_update_nv_cnfg        0x6B
#define NM_update_nv_cnfg_fail   0x0B
#define NM_update_nv_cnfg_succ   0x2B

typedef struct {
    byte        code;
    nv_struct   nv_cnfg;
} NM_query_nv_cnfg_response;

#define NM_query_nv_cnfg        0x68
#define NM_query_nv_cnfg_fail   0x08
#define NM_query_nv_cnfg_succ   0x28

typedef enum {
    APPL_OFFLINE   = 0,             /* Soft offline state                   */
    APPL_ONLINE  =1,
    APPL_RESET   =2,
    CHANGE_STATE =3
} nm_node_mode;

typedef enum {
    APPL_UNCNFG    = 2,
    NO_APPL_UNCNFG = 3,
    CNFG_ONLINE    = 4,
    CNFG_OFFLINE   = 6,             /* Hard offline state                   */
    SOFT_OFFLINE   = 0xC
} nm_node_state;


// karl's test
typedef struct {
    byte        code;
    byte        mode;               /* Interpret with 'nm_node_mode'        */
    byte        node_state;         /* Optional field if mode==CHANGE_STATE */
                                    /* Interpret with 'nm_node_state'       */
} NM_set_node_mode_request;




#define NM_set_node_mode 0x6C

typedef enum {
    ABSOLUTE           = 0,
    READ_ONLY_RELATIVE = 1,
    CONFIG_RELATIVE    = 2,
} nm_mem_mode;

typedef enum {
    NO_ACTION      = 0,
    BOTH_CS_RECALC = 1,
    CNFG_CS_RECALC = 4,
    ONLY_RESET     = 8,
    BOTH_CS_RECALC_RESET = 9,
    CNFG_CS_RECALC_RESET = 12,
} nm_mem_form;

typedef struct {
    byte    code;
    byte    mode;
    byte    offset_hi;
    byte    offset_lo;
    byte    count;
    byte    form;                    // followed by the data
} NM_write_memory_request;

#define NM_write_memory 0x6E


typedef struct {
    byte    code;
    byte    mode;
    byte    offset_hi;
    byte    offset_lo;
    byte    count;
} NM_read_memory_request;

#define NM_read_memory 0x6D


typedef struct {
    byte    length_hi;
    byte    length_lo;
    byte    num_netvars;
    byte    version;         // version 0 format
    byte    mtag_count;
} snvt_struct_v0;

typedef struct {
    byte    length_hi;
    byte    length_lo;
    byte    num_netvars_lo;
    byte    version;          // version 1 format
    byte    num_netvars_hi;
    byte    mtag_count;
} snvt_struct_v1;

// Partial list of SNVT type index values

typedef enum {
    SNVT_str_asc  = 36,
    SNVT_lev_cont = 21,
    SNVT_lev_disc = 22,
    SNVT_count_f  = 51,
} SNVT_t;

typedef struct {
    bits    nv_config_class     :1;
    bits    nv_auth_config      :1;
    bits    nv_priority_config  :1;
    bits    nv_service_type_config :1;
    bits    nv_offline          :1;
    bits    nv_polled           :1;
    bits    nv_sync             :1;
    bits    ext_rec             :1;
    bits    snvt_type_index     :8;        // use enum SNVT_t
} snvt_desc_struct;

typedef struct {
#ifdef   _MSC_VER
    byte        mask;          // Microsoft C does not allow odd-length
#else                          // bit fields
    bits    unused :3;
    bits    nc     :1;     // array count
    bits    sd     :1;     // self-documentation
    bits    nm     :1;     // network variable name
    bits    re     :1;     // rate estimate
    bits    mre    :1;     // max rate estimate
#endif
} snvt_ext_rec_mask;

// Network management message codes

#define NM_query_SNVT           0x72
#define NM_query_SNVT_fail      0x12
#define NM_query_SNVT_succ      0x32
#define NM_service_pin_message_code      0x7f

typedef struct  // karladded 07/28/00
{
byte neuron_id[NEURON_ID_LEN];
byte program_id[ID_STR_LEN];

} NM_service_pin_msg;

typedef struct {
    byte    code;
    byte    offset_hi;     // big-endian 16-bits
    byte    offset_lo;
    byte    count;
} NM_query_SNVT_request;

#define NM_wink 0x70

#define NM_NV_fetch         0x73
#define NM_NV_fetch_fail    0x13
#define NM_NV_fetch_succ    0x33

// Explicit application message codes

#define NA_appl_msg         0x00
#define NA_appl_offline     0x3F
#define NA_foreign_msg      0x40
#define NA_foreign_offline  0x4F

// Structure used by host application to store network variables

typedef enum { NV_IN = 0, NV_OUT = 1 } nv_direction;

typedef struct {                         // structure to define NVs
    int             size;                // number of bytes
    nv_direction    direction;           // input or output
    const           char * name;         // name of variable
    void            ( * print_func )( byte * ); // routine to print value
    void            ( * read_func )( byte * );  // routine to read value
    byte            data[ MAX_NETVAR_DATA ];    // actual storage for value
} network_variable;


typedef struct
{
   byte         code;
   byte         index;
} short_index_struct;

typedef struct
{
   short_index_struct short_index;
   nv_struct           nv_cnfg_data;
} short_config_data_struct;


typedef struct
{
    byte            code;
    byte            index_escape;   /* Must be 255 */
    word          index;
} long_index_struct;


typedef struct
{
   long_index_struct index;
   nv_struct          nv_cnfg_data;
} long_config_data_struct;


/* Update Network Variable Configuration Request
 */
typedef union NM_update_nv_cnfg_request
{
   short_config_data_struct short_config_index;
   long_config_data_struct  long_config_index;
} NM_update_nv_cnfg_request;

/* Query Network Variable Configuration Request
 */
typedef union NM_query_nv_cnfg_request
{
   short_index_struct   short_index;
   long_index_struct long_index;
} NM_query_nv_cnfg_request;

/*  Network Variable Fetch Request
 */
typedef NM_query_nv_cnfg_request NM_nv_fetch_request;

/*  Network Variable Fetch Response
 */
typedef union NM_nv_fetch_response
{
   short_index_struct   short_index;
   long_index_struct    long_index;   /* followed by data */
} NM_nv_fetch_response;



#define NM_clear_status    0x53

#define NM_query_status    0x51

typedef struct status_struct
{
	word   xmit_errors;
	word   transaction_timeouts;
	word   rcv_transaction_full;
	word   lost_msgs;
	word   missed_msgs;
	byte   reset_cause;
	byte   node_state;
	byte   version_number;
	byte   error_log;
	byte   model_number;
} status_struct;


#define NM_checksum_recalculate 0x6f
typedef enum
{
	BOTH_CS = 1,
	CNFG_CS = 4,
} NM_checksum_recalc_request_type;

#define NM_update_addr_request_code   0x66

typedef struct NM_update_addr_request
{
    byte code;
    byte addr_index;
    byte type;
    bits member_or_node:7;
    bits domain: 1;
    bits retry :4;
    bits rpt_timer  :4;
    bits tx_timer :4;
    bits rcv_timer :4;
    byte group_or_subnet;    

} NM_update_addr_request_struct;


#define NM_update_domain_request_code   0x63
#define DOMAIN_ID_LEN  6
#define AUTH_KEY_LEN 6

typedef struct NM_update_domain_request
{
    byte code;
    byte domain_index;
    byte id[DOMAIN_ID_LEN];
    byte subnet;    
    bits node : 7;
    bits must_be_one :1;  // reversed for endian ness
    byte len;
    byte key[AUTH_KEY_LEN];

} NM_update_domain_request_struct;

#endif

