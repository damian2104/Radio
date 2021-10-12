TARGET: radio-proxy radio-client

CC	= cc
CFLAGS	= -Wall -Wextra -O2
LFLAGS	= -Wall

radio-proxy.o radio-client err.o common.o client_sockets.o client_structures.o server_sockets.o server_structures.o: err.h client_sockets.h client_structures.h common.h server_structures.h server_sockets.h

radio-proxy: radio-proxy.o err.o common.o server_structures.o server_sockets.o
	$(CC) $(LFLAGS) $^ -o $@
	
radio-client: radio-client.o err.o common.o client_structures.o client_sockets.o
	$(CC) $(LFLAGS) $^ -o $@

.PHONY: clean TARGET
clean:
	rm -f radio-proxy radio-client *.o *~ *.bak
