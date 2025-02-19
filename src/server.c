#include "../include/concurrency.h"

void handle_signal(int signal);
void sigchld_handler(int signal);

// Here I ignored the warning for terminate because I wanted
//  terminate to act as a global flag for handling SIGINT.
//  I couldn't think of an alternative to not using a
//  non-constant global flag that also avoids compiler
//  warnings.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static int terminate = 0;

void handle_signal(int signal)
{
    if(signal == SIGINT)
    {
        terminate = 1;
    }
}

// sig handler for SIGCHLD
void sigchld_handler(int signal)
{
    int saved_errno = errno;    // save errno to avoid interference with other code
    (void)signal;               // unused parameter
    while(waitpid(-1, NULL, WNOHANG) > 0)
    {
        // Reap all exited child processes
    }
    errno = saved_errno;    // Restore errno
}

int main(int argc, char *argv[])
{
    struct socket_network net_socket;

    // int total_pid = 0;
    int err = 0;

    handle_arguments(argc, argv, &net_socket, &err);
    if(err != 0)
    {
        goto done;
    }

    socket_create(&net_socket, &err);
    if(err != 0)
    {
        goto done;
    }

    socket_set_non_blocking(&net_socket, &err);
    if(err != 0)
    {
        goto cleanup;
    }

    setup_network_address(&net_socket, &err);
    if(err != 0)
    {
        goto cleanup;
    }

    socket_bind(net_socket.sockfd, (struct sockaddr *)(&(net_socket.addr)), net_socket.addr_len, &err);
    if(err != 0)
    {
        goto cleanup;
    }

    socket_listen(net_socket.sockfd, SOMAXCONN, &err);
    if(err != 0)
    {
        goto cleanup;
    }

    setup_signal(handle_signal, SIGINT, &err);
    if(err != 0)
    {
        goto cleanup;
    }

    setup_signal(sigchld_handler, SIGCHLD, &err);
    if(err != 0)
    {
        goto cleanup;
    }

    while(terminate != 1)
    {
        pid_t pid;
        char  buffer[BUFFER_SIZE];
        int  *accepted_fd_copy;
        int   accepted_fd = accept(net_socket.sockfd, (struct sockaddr *)(&(net_socket.addr)), &(net_socket.addr_len));
        if(accepted_fd < 0)    // Error checks
        {
            if(errno != EAGAIN)
            {
                perror("Error accepting connection");
                err = errno;
                goto cleanup;
            }
            else    // Non-blocking socket -> go next
            {
                continue;
            }
        }

        // if(total_pid >= MAX_PROCESSES)
        // {
        //     reap_child(&total_pid, &err);
        //     if(err != 0)
        //     {
        //         break;
        //     }
        // }

        // Create a temp copy that will not be overwritten
        accepted_fd_copy = (int *)malloc(sizeof(int));
        if(accepted_fd_copy == NULL)
        {
            perror("malloc");
            err = errno;
            goto cleanup;
        }
        *accepted_fd_copy = accepted_fd;

        pid = fork();
        if(pid == 0)
        {
            char *client_argv[MAX_ARGS];
            int   is_builtin;
            int   client_exit = 0;
            char  full_path[BUFFER_SIZE];

            // Close network socket, uneeded
            close(net_socket.sockfd);

            while(client_exit == 0)
            {    // Handle client connection
                ssize_t bytes_received;

                memset(buffer, 0, sizeof(buffer));
                bytes_received = read(*accepted_fd_copy, buffer, BUFFER_SIZE);
                if(bytes_received <= 0)
                {
                    if(errno != EAGAIN)
                    {
                        perror("read");
                    }
                    else    // Non-blocking socket -> go next
                    {
                        continue;
                    }
                }

                // tokenize arguments
                client_exit = tokenize_client_args(client_argv, buffer, &err);
                if(err != 0)
                {
                    break;
                }

                // if cmd is builtin
                is_builtin = is_builtin_cmd(client_argv[0], full_path, &err);
                if(is_builtin == 1 && err == 0)
                {
                    printf("cmd is built-in.\n");
                }
                else if(is_builtin == 0 && err == 0)
                {    // if cmd is not builtin
                    printf("cmd is NOT built-in.\n");
                }
                else if(err != 0)
                {
                    break;
                }
            }
            printf("exiting child process\n");
            socket_close(*accepted_fd_copy);
            free(accepted_fd_copy);
            exit(0);
        }
        else if(pid < 0)
        {
            perror("fork");
            err = errno;
            socket_close(*accepted_fd_copy);
            free(accepted_fd_copy);
            goto cleanup;
        }
        else
        {
            socket_close(*accepted_fd_copy);
            free(accepted_fd_copy);
            // ++total_pid;
        }
    }

    display("server ran successfully");

cleanup:

    socket_close(net_socket.sockfd);

done:
    return err;
}

// while(total_pid > 0)
// {
//     reap_child(&total_pid, &err);
//     if(err != 0)
//     {
//         break;
//     }
// }

// if(execv(, argv) == -1)
// {
//     perror("execv");
//     socket_close(accepted_fd);
//     continue;
// }
// memset(buffer, 0, sizeof(buffer));
// bytes_received = recv(accepted_fd, buffer, sizeof(buffer), 0);

//==============
// if(bytes_received <= 0)
// {
//     // Handle errors or client disconnection
//     if(bytes_received == 0)
//     {
//         printf("Client disconnected.\n");
//     }
//     else
//     {
//         perror("Error receiving data");
//     }
//     socket_close(accepted_fd);
//     continue;
// }
//==============

// printf("Received from client: %s\n", buffer);
// socket_close(accepted_fd);
