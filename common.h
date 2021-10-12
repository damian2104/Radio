#include <stdbool.h>
#include <linux/in.h>
#define DISCOVER 1
#define IAM 2
#define KEEP_ALIVE 3
#define AUDIO 4
#define METADATA 6

#define HEADERS_SIZE 4
#define BUF_SIZE 1000

bool check_obligatory_parameters(bool a, bool b, bool c);

short decode_number(char* buffer, int start);

void save_message_nr(short n, char* buffer, int start);

void copy_client(struct sockaddr_in* toClient, struct sockaddr_in* fromClient);

bool is_proper_number(char *buffer);

bool check_if_parameter_0(int parameter);

bool check_if_proper_m_parameter(char *buffer);