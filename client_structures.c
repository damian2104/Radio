#include "client_structures.h"
#include <string.h>

void initialize_parameters(struct Parameters* parameters) {
	parameters->timeout = 5;
}

void initialize_menu(struct Menu* menu) {
	menu->pointer = 0;
	menu->nrOfIntercessors = 0;
	strncpy(menu->findIntercessor, "Szukaj pośrednika", BUF_SIZE-1);
	strncpy(menu->end, "Koniec", BUF_SIZE-1);
	menu->metadata[0] = '\0';
	menu->isRadioOn = false;
}

int save_type_of_parameter(char *argument) {
	int type = NO_TYPE;
	if (strcmp(argument, "-H") == 0) {
		type = HOST;
	} else if (strcmp(argument, "-P") == 0) {
		type = PORT_UDP;
	} else if (strcmp(argument, "-p") == 0) {
		type = PORT;
	} else if (strcmp(argument, "-T") == 0) {
		type = TIMEOUT;
	}

	return type;
}

void parse_parameters(struct Parameters* parameters, int argc, char *argv[]) {
	bool hostSet = false, portSet = false, portUDPset = false;
	int i = 1;
	int parameterType;
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
				case PORT:
					if (!is_proper_number(argv[i])) {
						fatal("bad argument");
					}
					strncpy(parameters->port, argv[i], BUF_SIZE);
					portSet = true;
					break;
				case PORT_UDP:
					if (!is_proper_number(argv[i])) {
						fatal("bad argument");
					}
					strncpy(parameters->portUDP, argv[i], BUF_SIZE);
					portUDPset = true;
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
			}
		}
		i++;
	}

	if (!check_obligatory_parameters(hostSet, portSet, portUDPset)) {
		fatal("parameter error - obligatory parameters not set");
	}

}

void prepare_menu(struct Menu* menu, char* buffer) {
	strncpy(buffer, menu->findIntercessor, BUF_SIZE-1);
	if (menu->pointer == 0) {
		strcat(buffer, " *");
	}
	strcat(buffer, "\r\n");
	for (int i = 0; i < menu->nrOfIntercessors; i++) {
		strcat(buffer, menu->stations[i]);
		if (menu->pointer == i+1) {
			strcat(buffer, " *");
		}
		strcat(buffer, "\r\n");
	}
	strcat(buffer, menu->end);
	if (menu->nrOfIntercessors != 0) {
		if (menu->pointer == (menu->nrOfIntercessors + 1)) {
			strcat(buffer, " *");
		}
	} else {
		if (menu->pointer == 1) {
			strcat(buffer, " *");
		}
	}
	strcat(buffer, "\r\n");
	if (menu->metadata[0] != '\0') {
		strcat(buffer, menu->metadata);
		strcat(buffer, "\r\n");
	}
}

int check_command(char* buffer, int len) {
	int commandType = NO_TYPE;
	// maybe enter
	if (len == 2) {
		if (buffer[0] == 13 && buffer[2] == 0) {
			commandType = ENTER;
		}
		// maybe arrow
	} else if (len == 3) {
		if (strcmp(buffer, "\033[A") == 0) {
			commandType = UP_ARROW;
		}
		if (strcmp(buffer, "\033[B") == 0) {
			commandType = DOWN_ARROW;
		}
	}
	return commandType;
}