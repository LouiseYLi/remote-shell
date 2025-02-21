#include "../include/utils.h"
#include <stdlib.h>

void sigint_handler(int signal);

// Here I ignored the warning for terminate because I wanted
//  terminate to act as a global flag for handling SIGINT.
//  I couldn't think of an alternative to not using a
//  non-constant global flag that also avoids compiler
//  warnings.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static int terminate = 0;

void sigint_handler(int signal)
{
    if(signal == SIGINT)
    {
        terminate = 1;
    }
}

int main(int argc, char *argv[])
{
    struct socket_network net_socket;
    ssize_t               bytes_sent;
    char                  buffer[BUFFER_SIZE];
    char                  exit_request[EXIT_REQ_SIZE + INT_BUFFER_SIZE + 2];
    char                  err_str[INT_BUFFER_SIZE + 2];
    int                   err = 0;

    handle_arguments(argc, argv, &net_socket, &err);
    if(err != 0)
    {
        goto done;
    }

    // socket initialization
    socket_create(&net_socket, &err);
    if(err != 0)
    {
        goto done;
    }

    setup_network_address(&net_socket, &err);
    if(err != 0)
    {
        goto cleanup;
    }
    // end socket initialization

    // socket connect
    socket_connect(net_socket.sockfd, (struct sockaddr *)(&(net_socket.addr)), net_socket.addr_len, &err);
    if(err != 0)
    {
        goto cleanup;
    }

    setup_signal(sigint_handler, SIGINT, &err);
    if(err != 0)
    {
        goto cleanup;
    }

    while(terminate != 1)
    {
        ssize_t bytes_received;
        char    user_input[INPUT_SIZE + 1];

        memset(buffer, 0, BUFFER_SIZE);
        memset(user_input, 0, INPUT_SIZE + 1);

        printf("\nEnter a command:\n");
        // scanf("%100s", user_input);
        if(fgets(user_input, sizeof(user_input), stdin) != NULL)
        {
            remove_trailing_newline(user_input);
        }
        else
        {
            perror("fgets");
            err = errno;
            break;
        }

        bytes_sent = write(net_socket.sockfd, user_input, strlen(user_input));
        if(bytes_sent <= 0)
        {
            perror("write");
            err = errno;
        }

        // check if user input contains exit
        if(strstr(user_input, "exit") != NULL)
        {
            printf("exiting...\n");
            goto cleanup;
        }

        bytes_received = read(net_socket.sockfd, buffer, BUFFER_SIZE);
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
        // print shell output
        printf("%s", buffer);
    }
    // TODO: what if user sends exit request too? move this to different scope
    // graceful termination
    // convert err to string
    snprintf(err_str, sizeof(err_str), "%d", err);
    // combine "exit " and err_str
    snprintf(exit_request, sizeof(exit_request), "exit %s", err_str);
    bytes_sent = write(net_socket.sockfd, exit_request, strlen(exit_request));
    if(bytes_sent <= 0)
    {
        perror("write");
        err = errno;
    }
cleanup:
    close(net_socket.sockfd);
done:
    return err;
}
