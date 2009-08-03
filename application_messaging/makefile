INCLUDES=-I. -I../../include -I../ldv -I../ni -I../../database2 -I/usr/include/mysql -I/usr/local/mysql/include -I/usr/local/mysql/include/mysql
LIBDIR=-L. -L../../libs -L../../low_level_hardware_interface/serial -L../../database2  
LIBS= -lni
CPU_TYPE=`uname -m`
FLAGS=  -g -Wall -O2 -march=$(CPU_TYPE) -L/usr/local/mysql/lib/mysql -L/usr/local/mysql/lib -L/usr/lib/mysql 
CC=g++ 
LINKER=g++

all: libApplMsg.a
 
clean:
	rm -f *.o
	rm -f *.a
	rm -f core*


applmsg.o: applmsg.cpp 
	$(CC) applmsg.cpp -c $(INCLUDES) $(FLAGS)

applmsg_degc_products.o: applmsg_degc_products.cpp
	$(CC) applmsg_degc_products.cpp -c $(INCLUDES) $(FLAGS)


libApplMsg.a:  applmsg.o applmsg_degc_products.o
	ar -r libApplMsg.a applmsg.o applmsg_degc_products.o

