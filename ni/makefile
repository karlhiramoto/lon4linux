#########################################################
#makefile for NI project


.SUFFIXES:  .C .cpp .o .c


VPATH = ../ni/.

CC = g++
CPP = g++
CC_FLAGS =  -g  -Wall  
CPP_FLAGS = -g -Wall
CPP_INCLUDES = -I../../include -I../ldv
LINKER = ar
ni_OBJECTS = ni_msg.o

all: libni.a

libni.a: $(ni_OBJECTS)
	$(LINKER) -r libni.a $(LINKER_ENTRY) $(LINKER_FLAGS) $(ni_OBJECTS) $(ni_LIBS)


.c.o:
	$(CPP) -c $< $(CPP_FLAGS) $(CPP_DEFINES) $(CPP_INCLUDES)
.cpp.o:
	$(CPP) -c $< $(CPP_FLAGS) $(CPP_DEFINES) $(CPP_INCLUDES)

ni_msg.o: ../ldv/ldv.h ni_msg.h

clean:
	rm -f *.o
	rm -f libni.a

