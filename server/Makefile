CC      ?= $(CROSS_COMPILE)gcc
CFLAGS  ?= -Wall -Werror -O2
LDFLAGS ?=

# Added pthread for both compile and link 
CFLAGS  += -pthread
LDFLAGS += -pthread

.PHONY: all clean

all: aesdsocket

aesdsocket: aesdsocket.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f aesdsocket
	
	









