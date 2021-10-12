#include "server_structures.h"
#include "err.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

/* Function search for pattern in text and returns position after match */
int search(char* pat, char* txt, int startingPosition, int txtLen) {
	int M = strlen(pat);
	int N = txtLen;

	for (int i = startingPosition; i <= N - M; i++) {
		int j;
		for (j = 0; j < M; j++) {
			if (txt[i + j] != pat[j]) {
				break;
			}
			if (j == (M-1)) {
				return (i+j+1);
			}
		}
	}
	return -1;
}

void initialize_parameters(struct Parameters* parameters) {
	parameters->mode = MODE_CLIENTS_OFF;
	parameters->meta = false;
	parameters->timeout = 5;
	parameters->timeoutUDP = 5;
}

int save_type_of_parameter(char *argument) {
	int type = NO_TYPE;
	if (strcmp(argument, "-h") == 0) {
		type = HOST;
	} else if (strcmp(argument, "-r") == 0) {
		type = RESOURCE;
	} else if (strcmp(argument, "-p") == 0) {
		type = PORT;
	} else if (strcmp(argument, "-m") == 0) {
		type = META;
	} else if (strcmp(argument, "-t") == 0) {
		type = TIMEOUT;
	} else if (strcmp(argument, "-P") == 0) {
		type = UDP_PORT;
	} else if (strcmp(argument, "-B") == 0) {
		type = MULTI;
	} else if (strcmp(argument, "-T") == 0) {
		type = UDP_TIMEOUT;
	}

	return type;
}

void parse_parameters(struct Parameters* parameters, int argc, char *argv[]) {
	bool hostSet = false, resourceSet = false, portSet = false, portUDPset = false;
	int parameterType;
	int i = 1;
	while (i < argc) {
		// type of parameter
		if (i % 2 == 1) {
			parameterType = save_type_of_parameter(argv[i]);
			// parameter
		} else {
			switch (parameterType) {
				case NO_TYPE:
					fatal("parameter error");
					break;
				case HOST:
					strncpy(parameters->host, argv[i], BUF_SIZE);
					hostSet = true;
					break;
				case RESOURCE:
					strncpy(parameters->resource, argv[i], BUF_SIZE);
					resourceSet = true;
					break;
				case PORT:
					if (!is_proper_number(argv[i])) {
						fatal("bad argument");
					}
					strncpy(parameters->port, argv[i], BUF_SIZE);
					portSet = true;
					break;
				case META:
					if (!check_if_proper_m_parameter(argv[i])) {
						fatal("bad argument");
					}
					if (strcmp(argv[i], "yes") == 0) {
						parameters->meta = true;
					}
					break;
				case TIMEOUT:
					if (!is_proper_number(argv[i])) {
						fatal("bad argument");
					}
					parameters->timeout = atoi(argv[i]);
					if (check_if_parameter_0(parameters->timeout)) {
						fatal("timeout 0");
					}
					break;
				case UDP_PORT:
					if (!is_proper_number(argv[i])) {
						fatal("bad argument");
					}
					strncpy(parameters->portUDP, argv[i], BUF_SIZE);
					portUDPset = true;
					break;
				case MULTI:
					if (!is_proper_number(argv[i])) {
						fatal("bad argument");
					}
					strncpy(parameters->multi, argv[i], BUF_SIZE);
					break;
				case UDP_TIMEOUT:
					if (!is_proper_number(argv[i])) {
						fatal("bad argument");
					}
					parameters->timeoutUDP = atoi(argv[i]);
					if (check_if_parameter_0(parameters->timeoutUDP)) {
						fatal("timeout 0");
					}
					break;
			}
		}
		i++;
	}

	if (!check_obligatory_parameters(hostSet, resourceSet, portSet)) {
		fatal("parameter error - obligatory parameters not set");
	}

	if (portUDPset) {
		parameters->mode = MODE_CLIENTS_ON;
	}
}

/* Function prepares message with radio name to send to client */
void prepare_message_init(char* buffer, char* headers, struct sockaddr_in* clientAddress, int sock) {
	char radioName[BUF_SIZE];
	int pos = search("icy-name:", headers, 0, strlen(headers));
	if (pos == -1) {
		fatal("no radio name");
	} else {
		int i = 0, k = pos;
		while (headers[k] != '\r') {
			radioName[i] = headers[k];
			i++;
			k++;
		}
		radioName[i] = '\0';
		int len = strlen(radioName);
		save_message_nr(IAM, buffer, 0);
		save_message_nr(len, buffer, 2);

		for (int i = HEADERS_SIZE; i < HEADERS_SIZE + len; i++) {
			buffer[i] = radioName[i-HEADERS_SIZE];
		}

		len = sendto(sock, buffer, HEADERS_SIZE+len, 0,
					 (struct sockaddr *) clientAddress, sizeof(struct sockaddr_in));
		if (len < 0) {
			syserr("sendto");
		}

	}
}

bool is_saved_client(struct sockaddr_in* clientAddress, struct Client *clients, int i) {
	if (clientAddress->sin_port == clients[i].address.sin_port &&
		clientAddress->sin_addr.s_addr == clients[i].address.sin_addr.s_addr) {
		return true;
	} else {
		return false;
	}
}

void actualize_client(struct sockaddr_in* clientAddress, struct Client* clients) {
	bool isSaved = false;
	for (int i = 0; i < clientSlotsUsed; i++) {
		isSaved = is_saved_client(clientAddress, clients, i);
		if (isSaved) {
			clients[i].isActive = true;
			clients[i].lastCall = time(NULL);
			break;
		}
	}
	if (!isSaved) {
		copy_client(&clients[clientSlotsUsed].address, clientAddress);
		clients[clientSlotsUsed].isActive = true;
		clients[clientSlotsUsed].lastCall = time(NULL);
		clientSlotsUsed++;
	}
}

void parse_message(char* buffer, struct sockaddr_in* clientAddress, struct Client clients[], int sock, char* headers) {
	short command, length;
	command = decode_number(buffer, 0);
	length = decode_number(buffer, 2);

	if (length != 0) {
		fatal("bad length value");
	}

	switch (command) {
		case DISCOVER:
			actualize_client(clientAddress, clients);
			prepare_message_init(buffer, headers, clientAddress, sock);
			break;
		case KEEP_ALIVE:
			actualize_client(clientAddress, clients);
			break;
	}
}