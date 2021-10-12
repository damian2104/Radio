#include <bits/types/time_t.h>
#include <netinet/in.h>
#include "common.h"
#define MODE_CLIENTS_OFF 1
#define MODE_CLIENTS_ON 2
#define NO_TYPE 0
#define HOST 1
#define RESOURCE 2
#define PORT 3
#define META 4
#define TIMEOUT 5
#define UDP_PORT 6
#define MULTI 7
#define UDP_TIMEOUT 8

#define WAITING_FOR_INTEGER 1
#define READING_METADATA 2
#define READING_STREAM 3

int clientSlotsUsed;
char stdout_buf[BUF_SIZE+10];
char stderr_buf[BUF_SIZE+10];
int stdout_len;
int stderr_len;

// to save clients
struct Client {
	struct sockaddr_in address;
	bool isActive;
	time_t lastCall;
};

// automata to parse data with metadata
struct DataStructure {
	int metaInt;
	int state;
	int howManyBytesLeft;
};

// initial parameters
struct Parameters {
	int mode;
	char host[BUF_SIZE];
	char resource[BUF_SIZE];
	char port[BUF_SIZE];
	bool meta;
	int timeout;
	char portUDP[BUF_SIZE];
	char multi[BUF_SIZE];
	int timeoutUDP;
};

int search(char* pat, char* txt, int startingPosition, int txtLen);

void initialize_parameters(struct Parameters* parameters);

int save_type_of_parameter(char *argument);

void parse_parameters(struct Parameters* parameters, int argc, char *argv[]);

void parse_message(char* buffer, struct sockaddr_in* clientAddress, struct Client clients[], int sock, char* headers);

void prepare_message_init(char* buffer, char* headers, struct sockaddr_in* clientAddress, int sock);

bool is_saved_client(struct sockaddr_in* clientAddress, struct Client *clients, int i);

void actualize_client(struct sockaddr_in* clientAddress, struct Client clients[]);