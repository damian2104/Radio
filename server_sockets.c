#include "server_sockets.h"
#include "err.h"

int socket_connect_udp(char* portNum) {
	struct sockaddr_in server_address;
	int sock;
	sock = socket(AF_INET, SOCK_DGRAM, 0); // creating IPv4 UDP socket
	if (sock < 0) {
		syserr("socket");
	}
	server_address.sin_family = AF_INET; // IPv4
	server_address.sin_addr.s_addr = htonl(INADDR_ANY); // listening on all interfaces
	server_address.sin_port = htons((uint16_t)atoi(portNum)); // default port for receiving is PORT_NUM

	int optval = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, (const void *)&optval , sizeof(int));

	if (bind(sock, (struct sockaddr *) &server_address, (socklen_t) sizeof(server_address)) < 0) {
		syserr("bind");
	}
	fcntl(sock, F_SETFL, O_NONBLOCK); // set non-blocking

	return sock;
}

int socket_connect(char *host, char* port){
	int sock, err;
	struct addrinfo addr_hints;
	struct addrinfo *addr_result;

	memset(&addr_hints, 0, sizeof(struct addrinfo));
	addr_hints.ai_family = AF_INET; // IPv4
	addr_hints.ai_socktype = SOCK_STREAM;
	addr_hints.ai_protocol = IPPROTO_TCP;
	err = getaddrinfo(host, port, &addr_hints, &addr_result);
	if (err == EAI_SYSTEM) { // system error
		syserr("getaddrinfo: %s", gai_strerror(err));
	}
	else if (err != 0) { // other error (host not found, etc.)
		fatal("getaddrinfo: %s", gai_strerror(err));
	}

	sock = socket(addr_result->ai_family, addr_result->ai_socktype, addr_result->ai_protocol);
	if (sock < 0)
		syserr("socket");

	// connect socket to the server
	if (connect(sock, addr_result->ai_addr, addr_result->ai_addrlen) < 0)
		syserr("connect");

	freeaddrinfo(addr_result);
	return sock;
}