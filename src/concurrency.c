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
    // get first arg (command) from buffer
    token = strtok_r(buffer, " ", &saveptr);

    // set first element of client_argv to token
    client_argv[i++] = token;
    remove_trailing_newline(client_argv[0]);

    // if command is equal to exit
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
    // TODO: remove this
    printf("client arguments:\n");
    for(int x = 0; x < i; x++)
    {
        printf("%s ", client_argv[x]);
    }
    printf("\n");
    //=====
    client_argv[i] = NULL;
    return ret;
}

/*
    Removes trailing new line from a string.
*/
void remove_trailing_newline(char *str)
{
    size_t len = strlen(str);
    if(len > 0 && str[len - 1] == '\n')
    {
        str[len - 1] = '\0';    // Replace newline with null terminator
    }
}

int is_builtin_cmd(const char *cmd, int *err)
{
    char       *saveptr;
    const char *token;
    int         is_builtin    = 0;
    const char *env_path      = getenv("PATH");
    char       *env_path_copy = NULL;

    if(env_path == NULL)
    {
        perror("getenv");
        *err = 1;
        goto done;
    }

    // make a modifiable copy of path
    env_path_copy = strdup(env_path);
    if(env_path_copy == NULL)
    {
        perror("strdup");
        *err = errno;
        goto done;
    }

    printf("PATH: %s\n", env_path_copy);

    token = strtok_r(env_path_copy, ":", &saveptr);
    while(token != NULL)
    {
        if(find_cmd(token, cmd, err) == 1)
        {
            is_builtin = 1;
            break;
        }
        printf("%s ", token);
        token = strtok_r(NULL, ":", &saveptr);
    }

    free(env_path_copy);
done:
    return is_builtin;
}

int find_cmd(const char *path, const char *cmd, int *err)
{
    const struct dirent *entry;
    DIR                 *dir_p;
    int                  found = 0;

    dir_p = opendir(path);
    if(dir_p == NULL)
    {
        perror("opendir");
        *err = errno;
        goto done;
    }
    else
    {
        printf("opened directory: %s...\n", path);
        // readdir_r is deprecated, that's why i
        // cppcheck-suppress readdirCalled
        while((entry = readdir(dir_p)))
        {
            if(strcasecmp(entry->d_name, cmd) == 0)
            {
                printf("Found match! %s\n", entry->d_name);
                found = 1;
                break;
            }
        }
    }
    closedir(dir_p);
done:
    return found;
}
