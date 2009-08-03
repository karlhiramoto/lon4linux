#########################################################


.SUFFIXES: .cc .cxx .C .o .cpp


VPATH = ./


YACC = bison
LEX = flex
JAVA = gcj
CC = gcc
LINKER = ar
CPP = g++
CPP_FLAGS = -g
CC_INCLUDES = -I../../include
CPP_INCLUDES = -I. -I../../include
ldv-lib_OBJECTS = ldv.o

all: libLDV.a

libLDV.a: $(ldv-lib_OBJECTS)
	$(LINKER) -r libLDV.a $(LINKER_ENTRY) $(LINKER_FLAGS) $(ldv-lib_OBJECTS) $(ldv-lib_LIBS)

.cpp.o:
	$(CPP) -c $< $(CPP_FLAGS) $(CPP_DEFINES) $(CPP_INCLUDES)
.C.o:
	$(CPP) -c $< $(CPP_FLAGS) $(CPP_DEFINES) $(CPP_INCLUDES)
.cc.o:
	$(CPP) -c $< $(CPP_FLAGS) $(CPP_DEFINES) $(CPP_INCLUDES)
.cxx.o:
	$(CPP) -c $< $(CPP_FLAGS) $(CPP_DEFINES) $(CPP_INCLUDES)


ldv.o: ldv.h ldv.cpp

clean:
	rm -f *.o
	rm -f *.a
