#include "../include/utils.h"

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
        // int  *accepted_fd_copy;
        int accepted_fd = accept(net_socket.sockfd, (struct sockaddr *)(&(net_socket.addr)), &(net_socket.addr_len));
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

        // // Create a temp copy that will not be overwritten
        // accepted_fd_copy = (int *)malloc(sizeof(int));
        // if(accepted_fd_copy == NULL)
        // {
        //     perror("malloc");
        //     err = errno;
        //     socket_close(accepted_fd);
        //     break;
        // }
        // *accepted_fd_copy = accepted_fd;

        pid = fork();
        if(pid == 0)
        {
            char *client_argv[MAX_ARGS];
            char  full_path[BUFFER_SIZE];
            int   is_builtin;
            int   client_exit = 0;
            int  *accepted_fd_copy;

            // Close network socket, uneeded
            close(net_socket.sockfd);

            // Create a temp copy that will not be overwritten
            accepted_fd_copy = (int *)malloc(sizeof(int));
            if(accepted_fd_copy == NULL)
            {
                perror("malloc");
                err = errno;
                close(accepted_fd);
                goto fork_done;
            }
            *accepted_fd_copy = accepted_fd;

            socket_set_blocking(accepted_fd_copy, &err);
            if(err != 0)
            {
                client_exit = 1;
            }

            while(client_exit == 0)
            {    // Handle client connection
                ssize_t bytes_received;
                ssize_t bytes_sent;
                char    message[BUFFER_SIZE];

                // clear the buffers
                memset(buffer, 0, BUFFER_SIZE);
                memset(full_path, 0, BUFFER_SIZE);
                memset(message, 0, BUFFER_SIZE);

                clear_array(client_argv);

                bytes_received = read(*accepted_fd_copy, buffer, BUFFER_SIZE);
                if(bytes_received <= 0)
                {
                    if(errno != EAGAIN)
                    {
                        perror("read");
                        err = errno;
                        break;
                    }
                    // Non-blocking socket -> go next
                    continue;
                }

                // tokenize arguments
                client_exit = tokenize_client_args(client_argv, buffer, &err);
                if(err != 0)
                {
                    break;
                }
                if(client_exit == 1)
                {
                    break;
                }
                // if an invalid cmd was sent
                if(client_exit == 2)
                {
                    const char *cmd_not_found = "command: not found\n";
                    bytes_sent                = write(*accepted_fd_copy, cmd_not_found, strlen(cmd_not_found));
                    if(bytes_sent <= 0)
                    {
                        if(errno != EAGAIN)
                        {
                            perror("write");
                            err = errno;
                            break;
                        }
                    }
                    client_exit = 0;
                    continue;
                }

                // if cmd is builtin
                is_builtin = is_builtin_cmd(client_argv[0], full_path, &err);
                if((is_builtin == 1 || is_builtin == -1) && err == 0)
                {
                    handle_builtin_cmd(full_path, client_argv, message, &err);

                    bytes_sent = write(*accepted_fd_copy, message, strlen(message));
                    if(bytes_sent <= 0)
                    {
                        if(errno != EAGAIN)
                        {
                            perror("write");
                            err = errno;
                            break;
                        }
                    }
                    client_exit = 0;
                }
                else if(is_builtin == 0 && err == 0)
                {    // if cmd is not builtin
                    handle_nonbuiltin_cmd(full_path, client_argv, *accepted_fd_copy, &err);
                }
                else if(err != 0)
                {
                    break;
                }
            }
            printf("closing forked process...\n");
            close(*accepted_fd_copy);
            free(accepted_fd_copy);
        fork_done:
            exit(err);
        }
        else if(pid < 0)
        {
            perror("fork");
            err = errno;
            // close(*accepted_fd_copy);
            // free(accepted_fd_copy);
            break;
        }
        else
        {
            // close(*accepted_fd_copy);
            // free(accepted_fd_copy);
            // ++total_pid;
        }
    }

    display("server ran successfully");

cleanup:

    close(net_socket.sockfd);

done:
    return err;
}
