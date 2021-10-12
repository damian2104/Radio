#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include "err.h"
#include "common.h"
#include "client_structures.h"

int socket_connect_udp(struct Parameters* parameters, struct sockaddr_in* myAddress);

int socket_connect(char* port);