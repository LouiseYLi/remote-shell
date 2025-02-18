#ifndef CONCURRENCY_H
#define CONCURRENCY_H

#include "../include/network_utils.h"

// void setup_sigchld_handler(int *err);

// void reap_child(int *total_pid, int *err);

void remove_trailing_newline(char *str);

int tokenize_client_args(char *client_argv[], char buffer[], int *err);

#endif    // CONCURRENCY_H
