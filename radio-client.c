#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include "err.h"
#include "common.h"
#include "client_sockets.h"

/* Function prepares message that cleans screen in telnet */
void prepare_clean_message(char *buffer) {
	strncpy(buffer, "\033[2J\033[1;1H", BUF_SIZE-1);
}

/* Function prepares message that sets character mode in telnet */
void prepare_character_mode_message(char *buffer) {
	strncpy(buffer, "\377\375\042\377\373\001", BUF_SIZE-1);
}

/* Function sends menu to telnet */
void print_menu(struct Menu *menu, int msgSock) {
	int len;
	char buffer[BUF_SIZE];
	prepare_clean_message(buffer);
	len = write(msgSock, buffer, sizeof(buffer));
	if (len < 0) {
		syserr("write");
	}
	prepare_menu(menu, buffer);
	len = write(msgSock, buffer, sizeof(buffer));
	if (len < 0) {
		syserr("write");
	}
}

/* Function saves info about radio server */
void save_radio(struct Menu* menu, char* buffer, struct sockaddr_in* serverAddress) {
	short command, length;
	command = decode_number(buffer, 0);
	length = decode_number(buffer, 2);
	int n = menu->nrOfIntercessors;
	if (command == IAM) {
		strncpy(menu->stations[n], buffer+HEADERS_SIZE, length);
		copy_client(&menu->servers[n], serverAddress);
		menu->nrOfIntercessors++;
	}
}

/* Function looks for radio intercessors */
void look_for_intercessors(struct Menu* menu, int sockUDP, struct sockaddr_in* myAddress) {
	int len, sflags = 0;
	char buffer[BUF_SIZE];
	save_message_nr(DISCOVER, buffer, 0);
	save_message_nr(0, buffer, 2);
	menu->nrOfIntercessors = 0;

	len = sendto(sockUDP, buffer, HEADERS_SIZE, sflags,
				 (struct sockaddr *) myAddress, sizeof(struct sockaddr_in));
	if (len < 0) {
		syserr("sendto");
	}
	len = 0;

}

void end_program(int sockUDP, int sockTCP) {
	shutdown(sockTCP, SHUT_RDWR);
	if (close(sockUDP) == -1) { //very rare errors can occur here, but then
		syserr("close"); //it's healthy to do the check
	}
	if (close(sockTCP) == -1) {
	}
	exit(0);
}

/* Function sets new radio */
void change_radio(struct Menu* menu, int pointer, int sockUDP) {
	int nr = pointer-1;
	int len, sflags = 0;
	char buffer[BUF_SIZE];

	save_message_nr(KEEP_ALIVE, buffer, 0);
	save_message_nr(0, buffer, 2);

	struct sockaddr_in *serverAddress = &menu->servers[nr];

	len = sendto(sockUDP, buffer, HEADERS_SIZE, sflags,
				 (struct sockaddr *) serverAddress, sizeof(struct sockaddr_in));
	if (len < 0) {
		syserr("sendto");
	}
	// time actualisation
	gettimeofday(&menu->lastInfoTime, NULL);
	gettimeofday(&menu->lastDataTime, NULL);
	menu->isRadioOn = true;
	copy_client(&menu->currentServer, serverAddress);
}

/* Function reacts to "ENTER" in telnet */
bool do_operation(struct Menu* menu, int sockUDP, int sockTCP, struct sockaddr_in* myAddress) {
	int pointer = menu->pointer;

	switch(pointer) {
		case 0:
			look_for_intercessors(menu, sockUDP, myAddress);
			break;
		default:
			if (pointer == menu->nrOfIntercessors+1) {
				return true;
				//end_program(sockUDP, sockTCP);
			} else if (pointer > 0 && pointer <= menu->nrOfIntercessors) {
				change_radio(menu, pointer, sockUDP);
				break;
			}
			break;
	}
	return false;
}

/* Function checks what command telnet sent */
bool parse_command(struct Menu* menu, int commandType, int sockUDP, int sockTCP , struct sockaddr_in* myAddress) {
	switch(commandType) {
		case NO_TYPE:
			break;
		case UP_ARROW:
			menu->pointer--;
			if (menu->pointer < 0) {
				menu->pointer = menu->nrOfIntercessors + 1;
			}
			break;
		case DOWN_ARROW:
			menu->pointer++;
			if (menu->pointer == (menu->nrOfIntercessors+2)) {
				menu->pointer = 0;
			}
			break;
		case ENTER:
			return do_operation(menu, sockUDP, sockTCP, myAddress);

	}
	return false;
}

/* Function prints audio on stdout */
void print_audio(char *buffer, int length) {
	if (write(1, buffer+HEADERS_SIZE, length) < 0) {
		syserr("write");
	}
}

/* Function saves metadata */
void print_meta(char* buffer, int length, struct Menu* menu) {
	strncpy(menu->metadata, buffer+HEADERS_SIZE, length);
}

/* Function checks message sent by server */
void parse_message(char* buffer, struct Menu* menu, struct sockaddr_in* serverAddress, int msgSock) {
	short command, length;

	command = decode_number(buffer, 0);
	length = decode_number(buffer, 2);

	switch (command) {
		case IAM:
			save_radio(menu, buffer, serverAddress);
			print_menu(menu, msgSock);
			break;
		case AUDIO:
			if (menu->currentServer.sin_port == serverAddress->sin_port &&
				menu->currentServer.sin_addr.s_addr == serverAddress->sin_addr.s_addr) {
				print_audio(buffer, length);
				gettimeofday(&menu->lastDataTime, NULL);
			}
			break;
		case METADATA:
			if (menu->currentServer.sin_port == serverAddress->sin_port &&
				menu->currentServer.sin_addr.s_addr == serverAddress->sin_addr.s_addr) {
				print_meta(buffer, length, menu);
				print_menu(menu, msgSock);
			}
			break;

	}

}

/* Function is responsible for operations related to server */
void radio_routine(struct Menu* menu, int sockUDP, struct sockaddr_in* myAddress, struct Parameters* parameters, int msgSock) {
	struct timeval t;
	int sflags = 0, len;
	char buffer[BUF_SIZE+10];
	struct sockaddr_in serverAddress;
	socklen_t rcvaLen;
	rcvaLen = (socklen_t) sizeof(struct sockaddr_in);

	len = recvfrom(sockUDP, buffer, BUF_SIZE+9, sflags,
				   (struct sockaddr *) &serverAddress, &rcvaLen);

	if (len > 0) {
		parse_message(buffer, menu, &serverAddress, msgSock);
	}

	if (menu->isRadioOn) {
		gettimeofday(&t, NULL);
		long sec = t.tv_sec - menu->lastInfoTime.tv_sec;
		long usec = t.tv_usec - menu->lastInfoTime.tv_usec;
		long diff = 1000000*sec + usec;
		// client sends KEEP_ALIVE after 3,5 seconds
		if (diff > 3500000) {
			save_message_nr(KEEP_ALIVE, buffer, 0);
			save_message_nr(0, buffer, 2);
			len = sendto(sockUDP, buffer, HEADERS_SIZE, sflags,
						 (struct sockaddr *) myAddress, sizeof(struct sockaddr_in));
			if (len < 0) {
				syserr(sendto);
			}
			gettimeofday(&menu->lastInfoTime, NULL);
		}
		sec = t.tv_sec - menu->lastDataTime.tv_sec;
		usec = t.tv_usec - menu->lastDataTime.tv_usec;
		diff = 1000000*sec + usec;
		// client disconnects if he doesn't get data for timeout seconds
		if (diff > 1000000*parameters->timeout) {
			menu->isRadioOn = false;
			menu->currentServer.sin_addr.s_addr = 0;
			menu->currentServer.sin_port = 0;
		}

	}
}

/* Function is responsible for comunication with telnet */
void telnet_routine(struct Parameters* parameters, int sockUDP, int sockTCP, struct sockaddr_in* myAddress) {
	int msgSock, commandType;
	struct sockaddr_in client_address;
	struct Menu menu;
	bool endConnection = false;
	socklen_t client_address_len;
	char buffer[BUF_SIZE];
	ssize_t len = 1, sndLen;
	initialize_menu(&menu);

	for (;;) {
		client_address_len = sizeof(client_address);
		// get client connection from the socket
		msgSock = accept(sockTCP, (struct sockaddr *) &client_address, &client_address_len);
		if (msgSock < 0) {
			syserr("accept");
		}
		fcntl(msgSock, F_SETFL, O_NONBLOCK);
		prepare_character_mode_message(buffer);
		sndLen = write(msgSock, buffer, sizeof(buffer));
		if (sndLen < 0) {
			syserr("write");
		}
		print_menu(&menu, msgSock);
		do {
			len = read(msgSock, buffer, BUF_SIZE-1);
			if (len == 0) {
				break;
			}
			if (len > 0) {
				buffer[len] = '\0';
				commandType = check_command(buffer, len);
				endConnection = parse_command(&menu, commandType, sockUDP, msgSock, myAddress);
				if (endConnection) {
					break;
				}
				print_menu(&menu, msgSock);
			}
			radio_routine(&menu, sockUDP, myAddress, parameters, msgSock);
		} while (true);
		close(msgSock);
		if (endConnection) {
			close(sockUDP);
			shutdown(sockTCP, SHUT_RDWR);
			close(sockTCP);
			exit(0);
		}
	}

}

int main(int argc, char *argv[]) {
	struct sockaddr_in myAddress;
	int sockUDP, sockTCP;
	struct Parameters parameters;
	initialize_parameters(&parameters);
	parse_parameters(&parameters, argc, argv);

	sockUDP = socket_connect_udp(&parameters, &myAddress);
	sockTCP = socket_connect(parameters.port);

	telnet_routine(&parameters, sockUDP, sockTCP, &myAddress);
	return 0;
}
