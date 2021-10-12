#include <netinet/in.h>
#include "common.h"
#include <string.h>

bool check_obligatory_parameters(bool a, bool b, bool c) {
	return (a && b && c);
}

/* Function checks number sent in packet */
short decode_number(char* buffer, int start) {
	short nh = 0, n;
	for (int i = start; i <= start+1; i++) {
		nh <<= 8;
		nh |= buffer[i];
	}
	n = ntohs(nh);
	return n;
}

/* Function saves message in buffer */
void save_message_nr(short n, char* buffer, int start) {
	short h = htons(n);
	buffer[start] = (h >> 8) & 0xFF;
	buffer[start+1] = h & 0xFF;
}

void copy_client(struct sockaddr_in* toClient, struct sockaddr_in* fromClient) {
	toClient->sin_addr.s_addr = fromClient->sin_addr.s_addr;
	toClient->sin_port = fromClient->sin_port;
	toClient->sin_family = fromClient->sin_family;
}

bool is_proper_number(char *buffer) {
	int len = strlen(buffer);
	for (int i = 0; i < len; i++) {
		if (buffer[i] < '0' || buffer[i] > '9') {
			return false;
		}
	}
	return true;
}

bool check_if_parameter_0(int parameter) {
	if (parameter == 0) {
		return true;
	}
	return false;
}

bool check_if_proper_m_parameter(char *buffer) {
	if (strcmp(buffer, "yes") == 0 || strcmp(buffer, "no") == 0) {
		return true;
	}
	return false;
}