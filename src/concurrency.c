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

    // set first element of client_argv to token
    client_argv[i++] = token;
    remove_trailing_newline(client_argv[0]);

    // if arg is equal to exit
    if(strcasecmp(client_argv[0], "exit") == 0)
    {
        ret = 1;
        printf("'exit' received\n... exiting\n");
        goto done;
    }

    // if arg is not one of the valid commands, exit
    if(strcasecmp(client_argv[0], "cd") != 0 && strcasecmp(client_argv[0], "pwd") != 0 && strcasecmp(client_argv[0], "echo") != 0 && strcasecmp(client_argv[0], "type") != 0)
    {
        ret = 1;
        printf("Invalid argument: %s... exiting.\n", client_argv[0]);
        goto done;
    }

    while(token != NULL)
    {
        if(i >= MAX_ARGS - 1)
        {
            perror("Too many arguments.");
            *err = 1;
            break;
        }
        // parse the same str
        token = strtok_r(NULL, " ", &saveptr);
        if(token == NULL)
        {
            break;
        }

        client_argv[i] = token;
        remove_trailing_newline(client_argv[i]);

        printf("ACTUAL TOKEN client_argv[%d] = \"%s\"\n", i, client_argv[i]);

        ++i;
    }

done:
    printf("client arguments:\n");
    for(int x = 0; x < i; x++)
    {
        printf("%s ", client_argv[x]);
    }
    printf("\n");
    client_argv[i] = NULL;
    return ret;
}

void remove_trailing_newline(char *str)
{
    size_t len = strlen(str);
    if(len > 0 && str[len - 1] == '\n')
    {
        str[len - 1] = '\0';    // Replace newline with null terminator
    }
}
