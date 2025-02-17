#include "../include/network_utils.h"

void handle_signal(int signal);

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

int main(int argc, char *argv[])
{
    struct socket_network net_socket;

    int return_val = 0;
    int err        = 0;

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

    setup_signal(handle_signal, &err);
    if(err != 0)
    {
        goto cleanup;
    }

    while(terminate != 1)
    {
        // TODO: should check if thread limit has reached before accepting another connection.
        //  if reached, then we should wait for one to join
        // Accept connection
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
        // Create a temp copy that will not be overwritten
        // accepted_fd_copy  = (int *)malloc(sizeof(int));
        // *accepted_fd_copy = accepted_fd;
        // if(pthread_create(&threads[thread_count], NULL, start_routine, (void *)accepted_fd_copy) != 0)
        // {
        //     // If thread creation fails, close accepted connection and go next.
        //     perror("Error creating thread");
        //     socket_close(accepted_fd);
        //     continue;
        // }
        // thread_count++;
    }

    display("server ran successfully");

cleanup:
    socket_close(net_socket.sockfd);

done:
    return_val = err;
    return return_val;
}
