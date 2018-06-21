#super simple makefile
# requires CAENVMElib

CC	= g++
CFLAGS	= -Wall -g -DLINUX -fPIC -std=c++11 #$(shell root-config --cflags)
LDFLAGS = -lCAENVME #$(shell root-config --libs) $(shell root-config --glibs)
SOURCES = $(shell echo ./*cc)
OBJECTS = $(SOURCES: .cc=bla.o)
CPP	= main
#CPPa    = 2_board_readout
#change to CPPa to change name of the .o file

all: $(SOURCES) $(CPP)

$(CPP) : $(OBJECTS)
	$(CC) $(OBJECTS) $(CFLAGS) $(LDFLAGS) $(LIBS) -o $(CPP)

clean:
	rm $(CPP)
