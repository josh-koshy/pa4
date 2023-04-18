#
# Students' Makefile for the Malloc Lab
#

CC = gcc
CFLAGS = -g -Wall -O2 -m32

OBJS = mm.o memlib.o fsecs.o fcyc.o clock.o ftimer.o

all: mdriver_p1 mdriver_p2

mdriver_p1: $(OBJS) mdriver_p1.o
	$(CC) $(CFLAGS) -o mdriver_p1 $(OBJS) mdriver_p1.o
mdriver_p2: $(OBJS) mdriver_p2.o
	$(CC) $(CFLAGS) -o mdriver_p2 $(OBJS) mdriver_p2.o

mdriver_p1.o: mdriver_p1.c fsecs.h fcyc.h clock.h memlib.h config.h mm.h
mdriver_p2.o: mdriver_p2.c fsecs.h fcyc.h clock.h memlib.h config.h mm.h
memlib.o: memlib.c memlib.h
mm.o: mm.c mm.h memlib.h
fsecs.o: fsecs.c fsecs.h config.h
fcyc.o: fcyc.c fcyc.h
ftimer.o: ftimer.c ftimer.h config.h
clock.o: clock.c clock.h

clean:
	rm -f *~ *.o mdriver_p1 mdriver_p2
