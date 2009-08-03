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



#ifndef _APPLMSG_H
#define _APPLMSG_H	1



#define NUM_NVS 8
            /* Number of Network Variable table entries on this node */
	    

#define MAX_NETWORK_NODES 255

#define NO_CHECK 0x20   /* do not check response code */

#include "dbfunc.h"

#include "ni_msg.h"
#include "ni_typedef_msg.h"

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ctype.h>
#include <ncurses.h>

#include <unistd.h>
#include <string.h>

#include "applmsg_degc_products.h"

// sorry for the global variables

extern FILE *lon_msg_log; // log  messsages here for protocol dubugging


// Function headers

void get_node_service_msg(NeuronCalibrationItem *);
void print_neuron_status_struct(status_struct *SS);

bool update_address(SendAddrDtl  *send_addr,NM_update_addr_request_struct *NewAddr);
bool update_NV_config(SendAddrDtl  *send_addr,NM_update_nv_cnfg_request *NVUpdate);
int read_neuron_NV(NeuronCalibrationItem*, byte ,long long*);
bool write_neuron_NV(NeuronCalibrationItem*, byte ,long long );
bool read_neuron_NV_float(NeuronCalibrationItem*, byte ,float*);
bool write_neuron_NV_float(NeuronCalibrationItem*, byte ,float);
long long read_neuron_memory_long_long(NeuronCalibrationItem *,nm_mem_mode,word offset,byte count);

bool program_NEIFile_to_neuron(NeuronCalibrationItem *CTSPtr,const char * NEIFile,int subnet, int node);

bool program_neuron_common_features(NeuronCalibrationItem *CTSPtr,const char * ImageFile,byte subnet, byte node);
void setup_NID_addrress(SendAddrDtl *send_addr,NeuronCalibrationItem  *CT);
void convert_int_to_byte_array(long long, byte* ,byte);
bool query_status(SendAddrDtl  *send_addr, status_struct *NeuronStatusPtr );
bool set_neuron_appless_unconfig_offline(NeuronCalibrationItem *CTSPtr);
bool set_node_online(NeuronCalibrationItem *CTSPtr);
bool get_num_net_vars(NeuronCalibrationItem *NCIPtr,int *num);
bool Get_NV_Config(NeuronCalibrationItem *NCIPtr, byte nv_index,	nv_struct   *nvStructData);
bool  Reset_Neuron(NeuronCalibrationItem *CTSPtr);


#endif
