#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H

#include "../include/display.h"
#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_PROCESSES 10
#define MAX_ARGS 10
#define INT_BUFFER_SIZE 10
#define EXIT_REQ_SIZE 5

struct socket_network
{
    char                   *address;
    int                     sockfd;
    struct sockaddr_storage addr;
    socklen_t               addr_len;
    uint16_t                port;
};

void setup_signal(void (*handler)(int), int signal_type, int *err);

void handle_arguments(int argc, char *argv[], struct socket_network *net_socket, int *err);

void socket_create(struct socket_network *net_socket, int *err);

void socket_set_non_blocking(struct socket_network *net_socket, int *err);

void setup_network_address(struct socket_network *net_socket, int *err);

void socket_bind(int sockfd, const struct sockaddr *addr, socklen_t addr_len, int *err);

void socket_listen(int sockfd, int backlog, int *err);

void socket_connect(int sockfd, const struct sockaddr *addr, socklen_t addr_len, int *err);

void socket_close(int sockfd);

// void accept_connection();

#endif    // NETWORK_UTILS_H
