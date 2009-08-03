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

#include "ldv.h"
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>


LDVCode
ldv_open( const char *device_name, LNI *pHandle )
{
    int handle;

    int len;
    struct sockaddr_in address;
    int result;
    int flag = 1;
  
    if (network_flag==true)
    {
        handle = socket(AF_INET, SOCK_STREAM, 0);
    
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = inet_addr(device_name);
        address.sin_port=6666;
    
        len = sizeof(address);
    
        result = connect(handle, (struct sockaddr *) &address, len);
    
        if (result ==-1)
        {
            perror("opps: Client Error on Connect. ");
    	    exit(1);
    	}   
	
        result = setsockopt(handle,              // socket affected 
                              IPPROTO_TCP,    // set option at TCP level 
                              TCP_NODELAY,     // name of option 
                              (char *) &flag,  // the cast is historical cruft 
                              sizeof(int));    // length of option value 

        if (result ==-1)
        {
            perror("opps: setsockopt ");
    	    exit(1);
    	}   
			      
    	if (fcntl(handle,F_SETFL, O_NONBLOCK) == -1 )
        {
            perror("opps: Client Error on fcntl in ldvpclta.c ");
    	    exit(1);
    	}   

	}
     else // network flag false
     {
      handle = open( device_name, O_RDWR | O_NONBLOCK );
      }
    
    if( handle < 0 )
    {
           // perror("TEST ldv_open() error is: ");
	    //printf("TEST ldv_open  handle=%d \n",handle);
	    //printf("TEST ldv_open device name=%s \n",device_name);

        switch( errno )
        {

            case EBUSY:
            case EACCES:
                return LDV_ALREADY_OPEN;
            case ENOENT:
            case ENAMETOOLONG:
            case ENOTDIR:
                return LDV_NOT_FOUND;
            case ENODEV:
                return LDV_NO_RESOURCES;
            default:
                return LDV_DEVICE_ERR;
        }
    }
    *pHandle = handle;
//    printf("SUCCESS ldv_open  handle=%d \n",handle);
//    printf("SUCCESS ldv_open device name=%s \n",device_name);

//    printf("Connected");

    return LDV_OK;
}

LDVCode ldv_close( LNI handle )
{
    if( 0 > close( handle ) )
        return LDV_INVALID_DEVICE_ID;
    return LDV_OK;
}

LDVCode ldv_read( LNI handle, void *pMsg, unsigned length )
{

    int ret_code=-999;
    unsigned char msg_length=0;
//    printf("\n ldv_read() START \n");


    if (network_flag==true)
        ret_code=read( handle, pMsg, 2 ); // get the ni command and length
    else
        ret_code=read( handle, pMsg, length );
	
//    printf("\n ldv_read() read call done. \n");
    
    if( 0 > ret_code )
    {
  //      printf("\n ldv_read() ERROR error=%d ret_code=%d\n",errno,ret_code);

        switch( errno )
        {
            case EISDIR:
            case EBADF:
            case EINVAL:
	        {
//			        printf("ldv_read()  LDV_INVALID_DEVICE_ID\n");
//  		        perror("Error is:");

                return LDV_INVALID_DEVICE_ID;
    		}
            case EINTR:
            case EAGAIN:
            case ENODATA:   ///  Karl's  hack.  
	        {
//		        printf("ldv_read() LDV_NO_MSG_AVAIL\n");
//  		        perror("Error is:");

                return LDV_NO_MSG_AVAIL;
		    }
            case EFAULT:
                return LDV_INVALID_BUF_LEN;
            case EBUSY:
                return LDV_DEVICE_BUSY;
            default:
             {
  	//	        printf("ldv_read() LDV_DEVICE_ERR \n");

 // 		        perror("Error is:");
                return LDV_DEVICE_ERR;
		     }
        }
    }
   
   if (network_flag==false)
        return LDV_OK;

    //there is more stuff to read.
  //  msg_length=((unsigned char)pMsg+1);
    memcpy(&msg_length,(char *)pMsg+1,1);
    
    if (msg_length==0)
        return LDV_OK;
    
//    printf("\n Reading %d more bytes\n",msg_length);

// maybe we should  block on this read
    ret_code=read( handle, (unsigned char *) pMsg+2,msg_length); // get the ni command and length
    
//    printf("\n ldv_read() SUCCESS.  Length=%d \n",ret_code);

    return LDV_OK;
}

LDVCode ldv_write( LNI handle, void *pMsg, unsigned length )
{
    int ret_code=-999;

//    printf("TEST ldv_write()  Length = %d ; Handle = %d\n",length,handle);
    
    ret_code= write( handle, pMsg, length );
    
//    printf("TEST ldv_write()  RET_CODE = %d\n",ret_code);

    
    if( 0 > ret_code )
    {
 //       printf("TEST ldv_read() handle = %d\n",handle);
 //       perror("TEST ldv_write() error is ");
//	    printf("TEST ldv_write()  errno = %d\n",errno);

        switch( errno )
        {
            case EISDIR:
            case EBADF:
            case EINVAL:
                return LDV_INVALID_DEVICE_ID;
            case EINTR:
            case EAGAIN:
                return LDV_NO_MSG_AVAIL;
            case EFAULT:
                return LDV_INVALID_BUF_LEN;
            case EBUSY:
                return LDV_DEVICE_BUSY;
            default:
                return LDV_DEVICE_ERR;
        }
    }
    return LDV_OK;
}
