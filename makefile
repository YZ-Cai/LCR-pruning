CC	= g++
CPPFLAGS= -Wno-deprecated -std=c++11 -O3 -m64 -c -w #-Wall
LDFLAGS	= -O3 -m64 
SOURCES	= main.cc
OBJECTS	= $(SOURCES:.cc=.o)
EXECUTABLE=main

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE) : $(OBJECTS)
	$(CC) $(LDFLAGS) $@.o -o $@

.cc.o : 
	$(CC) $(CPPFLAGS) $< -o $@

clear:
	rm -f *.o
