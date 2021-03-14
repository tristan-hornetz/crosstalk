override CFLAGS += -O3 -pthread -Wno-attributes -D _GNU_SOURCE
CC=gcc

SOURCES := $(wildcard *.c)
BINARIES := $(SOURCES:%.c=%.elf)

all: $(BINARIES)
	rm -f tsx_available

tsx_available:
	cp tsx_available._c tsx_available.c
	$(CC) -D $(notsx)  tsx_available.c -o $@
	rm tsx_available.c


%.o: %.c tsx_available
	$(CC) -D $(shell ./tsx_available)  -c $< -o $@ -m64 -static $(CFLAGS)

%.elf: %.o ../primitives/basic_primitives.o
	$(CC) $^ -o $@ -m64 -static $(CFLAGS)
	mv $@ ../demo_$*

clean:
	rm -f *.o *.so *.a
	rm -f ../demo_*
