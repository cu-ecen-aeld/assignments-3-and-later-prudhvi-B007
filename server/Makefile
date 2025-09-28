CC      ?= gcc            
CFLAGS  ?= -Wall -Werror -O2
LDFLAGS ?=

.PHONY: all clean

all: aesdsocket

aesdsocket: aesdsocket.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f aesdsocket

