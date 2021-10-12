#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include "err.h"
#include <signal.h>
#include <poll.h>
#include <time.h>
#include "common.h"
#include "server_structures.h"
#include "server_sockets.h"

/* function to catch SIGINT */
static volatile sig_atomic_t keep_running = 1;
static void sig_handler(int sig) {
    keep_running = 0;
    fprintf(stderr, "Signal %d catched. SHUT DOWN EVERYTHING!\n", sig);
}

/* Functions creates message to send it to radio server */
void create_message(struct Parameters* parameters, char *message) {
    char *part1 = "GET ";
    char *part2 = " HTTP/1.1\r\nHost: ";
    char *part3 = "\r\nIcy-MetaData: 1";
    char *part4 = "\r\nConnection: close\r\n\r\n";

    strcat(message, part1);
    strcat(message, parameters->resource);
    strcat(message, part2);
    strcat(message, parameters->host);
    if (parameters->meta) {
        strcat(message, part3);
    }
    strcat(message, part4);
}

bool check_if_headers_read(char* buffer, int length) {
    int index = search("\r\n\r\n", buffer, 0, length);
    if (index == -1) {
        return false;
    }
    return true;
}

/* copy header to another buffer */
void copy_headers(char* headers, char *buffer, int position) {
    strncpy(headers, buffer, position);
}

/* function to print data if metadata not set */
void print(char* buffer, int start, int end, struct Parameters* parameters) {
	if (parameters->mode == MODE_CLIENTS_OFF) {
		for (int i = start; i < end; i++) {
			printf("%c", buffer[i]);
		}
	} else {
		stdout_len = HEADERS_SIZE+end-start;
		save_message_nr(AUDIO, stdout_buf, 0);
		save_message_nr(end-start, stdout_buf, 2);
		for (int i = HEADERS_SIZE; i < stdout_len; i++) {
			stdout_buf[i] = buffer[start+i-HEADERS_SIZE];
		}
	}
}

/* Function looks for integer responsible for audio packet length */
bool check_metadata_and_find_integer(char* headers, int stringLen, int* metaint) {
    int position;
    char metaIntChar[BUF_SIZE] = "";
    position = search("icy-metaint:", headers, 0, stringLen);
    if (position == -1) {
        return false;
    } else {
        int i = position, j = 0;
        while (headers[i] != '\r') {
            metaIntChar[j] = headers[i];
            i++;
            j++;
        }
        metaIntChar[j] = '\0';
        sscanf(metaIntChar, "%d", metaint);
        return true;
    }
}

bool check_if_status_code_200(char *buffer, int length) {
    int firstNewLine = search("\n", buffer, 0, length);
    if (search("200 OK", buffer, 0, firstNewLine) == -1) {
        return true;
    }
    return false;
}

/* Function responsible for printing data with metadata */
void parse_data(char *buffer, struct DataStructure* data, int start, int end, struct Parameters* parameters) {
    int k;
    int outLen = HEADERS_SIZE, errLen = HEADERS_SIZE;

    for (int i = start; i < end; i++) {
        switch (data->state) {
            case WAITING_FOR_INTEGER:
            	k = (int) buffer[i];
                if (k != 0) {
                    //printf("INTEGER FOUND: %d\n", k);
                    data->howManyBytesLeft = 16 * k;
                    data->state = READING_METADATA;
                } else {
                    data->howManyBytesLeft = data->metaInt;
                    data->state = READING_STREAM;
                }
                break;
            case READING_METADATA:
            	if (parameters->mode == MODE_CLIENTS_OFF) {
					fprintf(stderr, "%c", buffer[i]);
				} else {
            		stderr_buf[errLen] = buffer[i];
            		errLen++;
            	}
                data->howManyBytesLeft--;
                if (data->howManyBytesLeft == 0) {
                    data->state = READING_STREAM;
                    data->howManyBytesLeft = data->metaInt;
                }
                break;
            case READING_STREAM:
				if (parameters->mode == MODE_CLIENTS_OFF) {
					printf("%c", buffer[i]);
				} else {
					stdout_buf[outLen] = buffer[i];
					outLen++;
				}

                data->howManyBytesLeft--;
                if (data->howManyBytesLeft == 0) {
                    data->state = WAITING_FOR_INTEGER;
                }
                break;
        }
    }

	if (parameters->mode == MODE_CLIENTS_ON) {
		stdout_len = outLen;
		stderr_len = errLen;
		save_message_nr(AUDIO, stdout_buf, 0);
		save_message_nr(stdout_len-HEADERS_SIZE, stdout_buf, 2);
		save_message_nr(METADATA, stderr_buf, 0);
		save_message_nr(stderr_len-HEADERS_SIZE, stderr_buf, 2);
	}

}

void setup_pollfd(struct pollfd fds[], int socket) {
    fds[0].fd = socket;
    fds[0].events = 0;
    fds[0].events |= POLLIN;
}

/* Function waits to timeout seconds for next server packet */
void check_if_response_arrived(struct pollfd *fds, int timeout) {
    if (poll(fds, 1, 1000*timeout) == 0) {
        fatal("Server not responding");
    }
}

void initialize_clients(struct Client *clients) {
	for (int i = 0; i < BUF_SIZE; i++) {
		clients->isActive = false;
	}
}

void mark_not_active_clients(struct Client clients[], int timeout) {
	time_t actualTime;
	for (int i = 0; i < clientSlotsUsed; i++) {
		actualTime = time(NULL);
		// mark not active if timeout passed without signal
		if (actualTime-clients[i].lastCall > timeout) {
			clients[i].isActive = false;
		}
	}
}

void send_data_to_clients(struct Client clients[], int sock, struct Parameters* parameters) {
	int sflags = 0, len;

	for (int i = 0; i < clientSlotsUsed; i++) {
		struct sockaddr_in *clientAddress = &clients[i].address;
		if (clients[i].isActive) {
			len = sendto(sock, stdout_buf, stdout_len, sflags,
						 (struct sockaddr *) clientAddress, sizeof(struct sockaddr_in));
			if (len < 0) {
				syserr("sendto");
			}

			if (parameters->meta && stderr_len > 4) {
				len = sendto(sock, stderr_buf, stderr_len, sflags,
							 (struct sockaddr *) clientAddress, sizeof(struct sockaddr_in));
				if (len < 0) {
					syserr("sendto");
				}
			}
		}
	}
}

/* Function to take care of clients */
/*
 * if you unblock commented area and in parse_message() and send_data_to_clients change parameter "sock" to "sockUDPhelp",
 * it will be possible to use more than one server on the same address and port, because main udp socket will be only used to receive
 * message and the help socket will be used to send data to client
 */
void serve_clients(struct Client *clients, int sock, int sockUDPhelp, char* headers, struct Parameters* parameters) {
	char buffer[BUF_SIZE];
	struct sockaddr_in clientAddress;
	socklen_t rcvaLen = (socklen_t) sizeof(clientAddress);
	int flags = 0, len = 0;

	// get all messages from clients
	do {
		len = recvfrom(sock, buffer, BUF_SIZE-1, flags,
					   (struct sockaddr *) &clientAddress, &rcvaLen);
		if (len > 0) {
			buffer[len] = '\0';
			parse_message(buffer, &clientAddress, clients, sock, headers);
		}
	} while (len >= 0);

	/*do {
		len = recvfrom(sockUDPhelp, buffer, BUF_SIZE, flags,
					   (struct sockaddr *) &clientAddress, &rcvaLen);
		if (len > 0) {
			buffer[len] = '\0';
			parse_message(buffer, &clientAddress, clients, sockUDPhelp, headers);
		}
	} while (len >= 0);*/

	mark_not_active_clients(clients, parameters->timeoutUDP);
	send_data_to_clients(clients, sock, parameters);
}


int main(int argc, char *argv[]) {
	clientSlotsUsed = 0;
	struct Client clients[BUF_SIZE];
    struct pollfd fds[1];
    bool headersRead = false, metaData = false;
    char radioData[BUF_SIZE], headers[BUF_SIZE];
    int socketTCP, socketUDP, socketUDPhelp, messageLength, metaint = -1;
    char message[BUF_SIZE] = "";
    struct Parameters parameters;
    struct DataStructure data;

    initialize_parameters(&parameters);
    parse_parameters(&parameters, argc, argv);
	initialize_clients(clients);

    socketTCP = socket_connect(parameters.host, parameters.port);
    socketUDP = socket_connect_udp(parameters.portUDP);
	socketUDPhelp = socket(PF_INET, SOCK_DGRAM, 0);
	fcntl(socketUDPhelp, F_SETFL, O_NONBLOCK); // set non-blocking
    setup_pollfd(fds, socketTCP);
    create_message(&parameters, message);
    signal(SIGINT, sig_handler);
    messageLength = strlen(message);
    if (write(socketTCP, message, messageLength) != messageLength) {
        syserr("write");
    }
    int currentPosition = 0;
    bzero(radioData, BUF_SIZE);

    // take header
	check_if_response_arrived(fds, parameters.timeout);
    while (!headersRead && (messageLength = read(socketTCP, radioData + currentPosition, BUF_SIZE - 1 - currentPosition)) != 0) {
        if (messageLength < 0) {
            syserr("read");
        }
        currentPosition += messageLength;
        headersRead = check_if_headers_read(radioData, messageLength);
        if (headersRead) {
            int position = search("\r\n\r\n", radioData, 0, currentPosition);
			copy_headers(headers, radioData, position);
            if (check_if_status_code_200(headers, currentPosition)) {
            	//printf(headers);
                fatal("Code not 200");
            }
            metaData = check_metadata_and_find_integer(headers, currentPosition, &metaint);
            if (metaData && !parameters.meta) {
            	fatal("unwanted metadata");
            }
            if (metaData) {
                data.metaInt = metaint;
                data.state = READING_STREAM;
                data.howManyBytesLeft = metaint;
				parse_data(radioData, &data, position, currentPosition, &parameters);
            } else {
                print(radioData, position, currentPosition, &parameters);
            }
            break;
        }
		check_if_response_arrived(fds, parameters.timeout);
    }
	check_if_response_arrived(fds, parameters.timeout);
    while ((messageLength = read(socketTCP, radioData, BUF_SIZE - 1)) != 0 && keep_running) {
        if (metaData) {
			parse_data(radioData, &data, 0, messageLength, &parameters);
        	if (parameters.mode == MODE_CLIENTS_ON) {
				serve_clients(clients, socketUDP, socketUDPhelp, headers, &parameters);
			}
        } else {
			print(radioData, 0, messageLength, &parameters);
        	if (parameters.mode == MODE_CLIENTS_ON) {
				serve_clients(clients, socketUDP, socketUDPhelp, headers, &parameters);
			}
        }
		check_if_response_arrived(fds, parameters.timeout);
    }

    // close all sockets
    shutdown(socketTCP, SHUT_RDWR);
    close(socketTCP);
    close(socketUDP);
    close(socketUDPhelp);
    return 0;
}
