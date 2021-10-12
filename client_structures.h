#include <netinet/in.h>
#include "common.h"
#include <string.h>
#include <stdio.h>
#include "err.h"
#include <stdlib.h>
#include <string.h>

#define NO_TYPE 0
#define HOST 1
#define PORT 2
#define PORT_UDP 3
#define TIMEOUT 4
#define QUEUE_LENGTH 10
#define HEADERS_SIZE 4

#define UP_ARROW 1
#define DOWN_ARROW 2
#define ENTER 3

// structure to save menu
struct Menu {
	char findIntercessor[BUF_SIZE];
	char end[BUF_SIZE];
	char stations[BUF_SIZE][BUF_SIZE];
	int nrOfIntercessors;
	int pointer;
	struct sockaddr_in servers[BUF_SIZE];
	struct sockaddr_in currentServer;
	bool isRadioOn;
	char metadata[BUF_SIZE];
	struct timeval lastInfoTime;
	struct timeval lastDataTime;
};

// initial parameters
struct Parameters {
	char host[BUF_SIZE];
	char port[BUF_SIZE];
	int timeout;
	char portUDP[BUF_SIZE];
};

void initialize_parameters(struct Parameters* parameters);

void initialize_menu(struct Menu* menu);

int save_type_of_parameter(char *argument);

void parse_parameters(struct Parameters* parameters, int argc, char *argv[]);

void prepare_menu(struct Menu* menu, char* buffer);

int check_command(char* buffer, int len);