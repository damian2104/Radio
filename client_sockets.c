#include "client_sockets.h"

int socket_connect_udp(struct Parameters* parameters, struct sockaddr_in* myAddress) {
	//printf("portNum: %s\n", parameters->portUDP);
	struct addrinfo addr_hints;
	struct addrinfo *addr_result;
	int sock;


	// 'converting' host/port in string to struct addrinfo
	(void) memset(&addr_hints, 0, sizeof(struct addrinfo));
	addr_hints.ai_family = AF_INET; // IPv4
	addr_hints.ai_socktype = SOCK_DGRAM;
	addr_hints.ai_protocol = IPPROTO_UDP;
	addr_hints.ai_flags = 0;
	addr_hints.ai_addrlen = 0;
	addr_hints.ai_addr = NULL;
	addr_hints.ai_canonname = NULL;
	addr_hints.ai_next = NULL;
	if (getaddrinfo(parameters->host, NULL, &addr_hints, &addr_result) != 0) {
		syserr("getaddrinfo");
	}

	myAddress->sin_family = AF_INET; // IPv4
	myAddress->sin_addr.s_addr =
			((struct sockaddr_in*) (addr_result->ai_addr))->sin_addr.s_addr; // address IP
	myAddress->sin_port = htons((uint16_t) atoi(parameters->portUDP)); // port from the command line

	freeaddrinfo(addr_result);

	sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		syserr("socket");
	}

	int optval = 1;
	setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (void*)&optval, sizeof (optval));
	fcntl(sock, F_SETFL, O_NONBLOCK); // set non-blocking
	return sock;
}

int socket_connect(char* port){
	int sock;
	struct sockaddr_in server_address;

	sock = socket(PF_INET, SOCK_STREAM, 0); // creating IPv4 TCP socket
	if (sock < 0) {
		syserr("socket");
	}
	server_address.sin_family = AF_INET; // IPv4
	server_address.sin_addr.s_addr = htonl(INADDR_ANY); // listening on all interfaces
	server_address.sin_port = htons(atoi(port)); // listening on port PORT_NUM

	// bind the socket to a concrete address
	if (bind(sock, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
		syserr("bind");
	}
	// switch to listening (passive open)
	if (listen(sock, QUEUE_LENGTH) < 0) {
		syserr("listen");
	}

	return sock;
}