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

///////////////
#if !defined LDV_DEFINED
#define LDV_DEFINED

#define MAX_NI_DATA 256

typedef int LNI;
typedef enum
{
    LDV_OK = 0,
    LDV_NOT_FOUND,
    LDV_ALREADY_OPEN,
    LDV_DEVICE_ERR,
    LDV_INVALID_DEVICE_ID,
    LDV_DEVICE_BUSY,
    LDV_NO_MSG_AVAIL,
    LDV_NO_BUF_AVAIL,
    LDV_NO_RESOURCES,
    LDV_INVALID_BUF_LEN
} LDVCode;

extern bool network_flag;

LDVCode ldv_open( const char *device_name, LNI *pHandle );
LDVCode ldv_close( LNI handle );
LDVCode ldv_read( LNI handle, void *pMsg, unsigned length );
LDVCode ldv_write( LNI handle, void *pMsg, unsigned length );

#if defined SLTA_2

/***************************************************/
/* slta-2 specific not standard functions and data */
/***************************************************/

void ldv_post_events( void );

extern int auto_baud_feature;
extern int alert_ack_prtcl;
extern int ldv_buffers;
extern unsigned int baud_rate;


#endif /* SLTA_2 */

#endif /* LDV_DEFINED */
