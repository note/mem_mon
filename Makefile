CC=gcc
CPP=g++
CFLAGS=-Wall -I include/

all: memtrace tests

system.o: src/system.cpp
	$(CPP) $(CFLAGS) $< -c

process.o: src/process.cpp
	$(CPP) $(CFLAGS) $< -c

memtrace: src/memtrace.cpp
	$(CPP) $(CFLAGS) $< -o memtrace

tests: libtest mthreaded signals basic

libtest: libtest.o system.o process.o
	$(CPP) $(CFLAGS) libtest.o system.o process.o -o $@

basic: tests/basic.cpp
	$(CPP) $(CFLAGS) $< -lrt -static -o $@
	
libtest.o: tests/libtest.cpp
	$(CPP) $(CFLAGS) $< -c

mthreaded: tests/mthreaded.cpp
	$(CPP) $(CFLAGS) $< -lpthread -static -o $@

signals: tests/signals.cpp
	$(CPP) $(CFLAGS) -static $< -o $@

clean:
	-@rm process.o system.o libtest libtest.o mthreaded signals memtrace basic &> /dev/null || true

.PHONY: clean
