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

tests: libtest mthreaded signals

libtest: libtest.o system.o process.o
	$(CPP) $(CFLAGS) libtest.o system.o process.o -o $@
	
libtest.o: tests/libtest.cpp
	$(CPP) $(CFLAGS) $< -c

mthreaded: tests/mthreaded.cpp
	$(CPP) $(CFLAGS) -static -lrt -lpthread -o $@ $<

signals: tests/signals.cpp
	$(CPP) $(CFLAGS) -static $< -o $@

clean:
	-@rm process.o system.o libtest libtest.o mthreaded signals memtrace &> /dev/null || true

.PHONY: clean
