cmake_minimum_required(VERSION 3.16)
project(radio_proxy C)

set(CMAKE_C_STANDARD 99)

add_executable(radio_proxy radio-proxy.c err.c err.h radio-client.c common.h common.c client_sockets.c client_sockets.h client_structures.c client_structures.h server_structures.c server_structures.h server_sockets.c server_sockets.h)