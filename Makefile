CC=gcc
CPP=g++
CFLAGS=-Wall

all: test memtrace

test: test.o system.o process.o
	$(CPP) $(CFLAGS) test.o system.o process.o -o test
	
test.o: test.cpp
	$(CPP) $(CFLAGS) $< -c

system.o: system.cpp
	$(CPP) $(CFLAGS) $< -c

process.o: process.cpp
	$(CPP) $(CFLAGS) $< -c

memtrace: memtrace.cpp
	$(CPP) $(CFLAGS) $< -o memtrace

clean:
	-@rm test process.o system.o test.o memtrace &> /dev/null || true

.PHONY: clean
