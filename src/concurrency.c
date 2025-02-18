#include "../include/concurrency.h"

/*
    Tokenizes client arguments.
    Return 0 if ok, 1 if exit requested.
*/
int tokenize_client_args(char *client_argv[], char buffer[], int *err)
{
    char *saveptr;
    char *token;
    int   ret = 0;
    int   i   = 0;
    // get first arg from buffer
    token = strtok_r(buffer, " ", &saveptr);
    // while not null
    while(token != NULL)
    {
        // set first element of client_argv to token
        client_argv[i] = token;
        printf("token: %s\n", client_argv[i]);
        if(i >= MAX_ARGS - 1)
        {
            perror("too many arguments");
            *err = 1;
            break;
        }
        // if token is equal to exit
        if(strcasecmp(client_argv[i], "exit") == 0)
        {
            ret = 1;
            printf("'exit' received\n... exiting");
            break;
        }
        // if token is not one of the valid commands, set and exit
        if(!(strcasecmp(client_argv[i], "cd")) && !(strcasecmp(client_argv[i], "pwd")) && !(strcasecmp(client_argv[i], "echo")) && !(strcasecmp(client_argv[i], "type")))
        {
            ret = 1;
            printf("Invalid argument: %s... exiting.\n", client_argv[i]);
            printf("%d\n", strcasecmp(client_argv[i], "cd"));
            break;
        }

        // parse the same str
        token = strtok_r(NULL, " ", &saveptr);
        ++i;
    }
    client_argv[i] = NULL;

    return ret;
}
