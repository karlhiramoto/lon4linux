#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <time.h>

// Transceiver Id Watcher
// Bit 2: Baud/16  Bit 1: Baud/2  Bit 0: 1=Single-ended 0=Differential
#define TP1250      0
#define TP78        4
#define FTT10       5
#define RS485_625   3   // Neuron site is Single-ended
#define RS485_39    7   // Neuron site is Single-ended

#define XCVR    FTT10

#define IOCTL_VERSION	0x43504C00
#define IOCTL_WATCHER	0x43504C01

#define CMD_LOAD        0
#define CMD_PACKET      1
#define CMD_CLEAR       2
#define CMD_TIME        3
#define CMD_START       4
#define CMD_STOP        5
#define CMD_GETTIME     6

typedef unsigned char	u8;
typedef unsigned short	u16;
typedef unsigned long	u32;

u8 gbuf[4096];
int	ni_handle;

static const char svctype[3][8][12] = {
	{ "ACKD      ","UNACKD_REP","ack       ","???       ",
	  "REMINDER  ","REM/MSG   ","???       ","???       " },
	{ "REQUEST   ","???       ","response  ","???       ",
	  "REMINDER  ","REM/MSG   ","???       ","???       " },
	{ "CHALLENGE ","???       ","reply     ","???       ",
	  "???       ","???       ","???       ","???       " }
};


// Show an error message and exit

void myerror(char *s)
{
	perror( s );
	close( ni_handle );
	exit( -1 );
}


// Load a file into a buffer and return its size

int loadfile( u8 *d, char *s )
{
	int fd;
	int size;

	fd = open( s, O_RDONLY );
	if( fd < 0 ) return -1;
	size = read( fd, d, 4096 );
	close( fd );
	return( size );
}


// Store the actual time in 102.4 us intervals since 01.01.1970
// in a 6 byte buffer ( Motorola order )

void get_time( u8 *d )
{
	struct timezone tz;
	struct timeval tv;
	long long t;
	u8   *p;

	tz.tz_minuteswest = 0;
	tz.tz_dsttime = 0;
	gettimeofday( &tv, &tz );
	t = tv.tv_sec;
	t = (t * 1000) + tv.tv_usec/1000;		// Milliseconds since 01.01.1970

	t *= 2500;
	t /= 256;								// 102.4 us intervals
	p = (u8 *)&t;
	*d++ = p[5]; *d++ = p[4]; *d++ = p[3];	// Store in Motorola order
	*d++ = p[2]; *d++ = p[1]; *d++ = p[0];
}


// Convert a 6 byte ( Motorola order ) timestamp
// in 102.4 us intervals at the END of a packet
// into millisecond intervals at packet START and display it

void show_time( u8 *s, int len, u8 xcvr )
{
	struct tm *lt;
	int  ms;
	time_t gmt;
	long long t;
	u8   *p;

	if( xcvr & 2 ) len *= 2;				// Packet duration depends
	if( !(xcvr & 1) ) len /= 16;			// on transceiver baud rate

	t = 0;
	p = (u8 *)&t;
	p[5] = *s++; p[4] = *s++; p[3] = *s++;	// Motorola -> Intel
	p[2] = *s++; p[1] = *s++; p[0] = *s++;
	t -= len;
	t *= 256;
	t /= 2500;								// Milliseconds

	ms  = t % 1000; gmt = (time_t)(t / 1000);
	lt = localtime( &gmt );
	printf( "\n%02d.%02d.%04d  %02d:%02d:%02d.%03d  ",
			lt->tm_mday, lt->tm_mon+1, lt->tm_year+1900,
			lt->tm_hour, lt->tm_min, lt->tm_sec, ms );
}


// Decode a packet from LonTalk format and display it
// For further datails see the LonTalk protocol specification

void show_packet( u8* buf, int count )
{
	u8  *p;
	int i;
	u8  pdufmt, pdutype, adrfmt, domlen;
	u8  snet, snode, dnet, dnode;

	p = buf;

// Media access control

	printf( "Backlog =%2d", *p & 0x3F );
	if( *p & 0x80 ) printf( " Prio" );
	if( *p & 0x40 ) printf( " AltPath" );
	printf( "\n" );
	p++;

// Address field

	pdufmt = (*p >> 4) & 3;
	adrfmt = (*p >> 2) & 3;
	domlen = *p++ & 3;
	domlen = (domlen * (domlen + 1)) / 2;
	snet   = *p++;
	snode  = *p++;
	dnet   = *p++;
	printf( "%3d/%-3d -> ", snet, snode & 0x7F );
	switch( adrfmt )
	{
		case 0:
			if( dnet ) printf( "Subnet %-3d      ", dnet );
			else       printf( "Broadcast       " );
			break;
		case 1:
			printf( "Group %-3d       ", dnet );
			break;
		case 2:
			dnode = *p++ & 0x7F;
			if( snode & 0x80 ) printf( "%3d/%-3d         ", dnet, dnode );
			else {
				printf( "%3d/%-3d G%-3d M%-2d", dnet, dnode, p[0], p[1] );
				p += 2;
			}
			break;
		case 3:
			printf( "NID " );
			for( i = 0; i < 6; i++ ) printf( "%02X", *p++ );
			break;
	}

// Domain field

	printf( " Domain: " );
	if( !domlen ) printf( "default     " );
	else {
		for( i = 0; i < domlen; i++ ) printf( "%02X", *p++ );
		for( i = domlen; i < 6; i++ ) printf( "  " );
	}

// Transaction

	if( pdufmt == 3 ) printf( "  - UNACKD " );
	else {
		printf( " %2d ", *p & 0x0F );
		pdutype = (*p++ >> 4) & 7;
		printf( svctype[ pdufmt ][ pdutype ] );
		if((pdutype == 4) || (pdutype == 5))
		{
			i = *p++;
			printf( "\nMList: " );
			while( i-- ) printf( "%02X", *p++ );
		}
	}

// Data field

	printf( "\n" );
	count -= (p - buf);
	if( count > 0 )
	{
		if((*p  & 0x80) && (count >= 2))
		{
			printf( "NV %02X%02X:", p[0] & 0x3F, p[1] );	// NV selector
			if(*p & 0x40) printf( " Poll");
			p += 2;
			count -= 2;
		}
		else {
			printf( "%02X:", *p++ );						// Message code
			count--;
		}
		while( count-- ) printf( " %02X", *p++ );			// data
		printf( "\n" );
	}
}


int main(int argc, char **argv)
{
	char devname[] = "/dev/lonx";
	char version[80];
	int result;
	int count;
	u8  *p;

	// Argument must be '1'...'9' for lon1...lon9

	if(argc < 2) {
		printf("Syntax: ./analyzer n (n=1...9)\n");
		return 1;
	}
	if((argv[1][0] < '1') || (argv[1][0] > '9')) {
		printf( "Argument must be between 1 and 9\n" );
		return 1;
	}
	devname[8] = argv[1][0];

	// Open the LON device

	ni_handle = open( devname, O_RDWR );
	if( ni_handle < 0 ) myerror( "LON device not found" );

	ioctl( ni_handle, IOCTL_VERSION, (unsigned long)&version );
	printf("%s\n", version);


	// Buffer format of watcher ioctl is:
	//   Length (hi & lo byte) of following data
	//   Watcher command
	//   Optional data

	// Load the firmware file

	result = loadfile( &gbuf[4], "lwafw.lwa" );
	if( result <= 0 ) myerror( "LWA firmware not found" );

	count = result + 2;
	gbuf[0] = count >> 8;
	gbuf[1] = count & 0xFF;
	gbuf[2] = CMD_LOAD;
	gbuf[3] = XCVR;
	result = ioctl( ni_handle, IOCTL_WATCHER, (u32)&gbuf );
	if( result != 2 ) myerror( "LWA hardware not found" );

	printf("Record...\n");

	// Set time

	gbuf[0] = 0;
	gbuf[1] = 7;
	gbuf[2] = CMD_TIME;
	get_time( &gbuf[3] );
	result = ioctl( ni_handle, IOCTL_WATCHER, (u32)&gbuf );

	// Start recording

	gbuf[0] = 0;
	gbuf[1] = 1;
	gbuf[2] = CMD_START;
	result = ioctl( ni_handle, IOCTL_WATCHER, (u32)&gbuf );

	while( 1 )
	{
		gbuf[0] = 0;
		gbuf[1] = 1;
		gbuf[2] = CMD_PACKET;
		result = ioctl( ni_handle, IOCTL_WATCHER, (u32)&gbuf );
		if( result < 2 ) myerror( "Device error" );

		p = gbuf;
		count = *p++;
		count = (count << 8) | *p++;
		if( count > 7 )
		{
			show_time( p, count - 5, XCVR );
			if( count < 12 ) printf( "too short\n" );
			else if( p[6] & 2 ) printf( "CRC-ERROR\n" );
			else show_packet( &p[7], count - 7 );
		}
		else poll( NULL, 0, 100 );
	}

	return 0;
}
