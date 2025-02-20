#ifndef CONCURRENCY_H
#define CONCURRENCY_H

#include "../include/network_utils.h"

// void setup_sigchld_handler(int *err);

// void reap_child(int *total_pid, int *err);

void remove_trailing_newline(char *str);

int tokenize_client_args(char *client_argv[], char buffer[], int *err);

int is_builtin_cmd(const char *cmd, char *full_path, int *err);

void find_cmd(const char *path, const char *cmd, char *full_path, int *err);

void handle_nonbuiltin_cmd(const char full_path[], char *client_argv[], int socket_fd, int *err);

void clear_array(char *client_argv[]);

int is_cmd_args_valid(char *client_argv[], char message[], int *err);

void handle_builtin_cmd(char *client_argv[], char message[], int *err);

void builtin_cd(const char *path, int *err);

#endif    // CONCURRENCY_H
