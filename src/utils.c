#include "../include/utils.h"

/*
    Tokenizes client arguments.
    Return 0 if ok, 1 if exit requested, 2 if an invalid argument.
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
        goto done;
    }

    // if arg is not one of the valid commands, exit
    if(strcasecmp(client_argv[0], "cd") != 0 && strcasecmp(client_argv[0], "pwd") != 0 && strcasecmp(client_argv[0], "echo") != 0 && strcasecmp(client_argv[0], "type") != 0)
    {
        ret = 2;
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

        ++i;
    }

done:
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

/*
    Checks if command is built-in or not.
    Returns 1 if built-in, 0 if not built-in, -1 else.

    cmd: Command to check whether it is built-in or not.
    full_path: Buffer to hold the full path of the exe, if found.
*/
int is_builtin_cmd(const char *cmd, char full_path[], int *err)
{
    char       *saveptr;
    const char *token;
    int         is_builtin    = 1;
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

    token = strtok_r(env_path_copy, ":", &saveptr);
    while(token != NULL)
    {
        find_cmd(token, cmd, full_path, err);
        if(strlen(full_path) > 0)
        {
            is_builtin = 0;
            break;
        }
        token = strtok_r(NULL, ":", &saveptr);
    }

    if(is_builtin != 0 && strcasecmp(cmd, "cd") != 0)
    {
        is_builtin = -1;
    }

    free(env_path_copy);
done:
    return is_builtin;
}

/*
    Attempts to find executable of cmd in path.
    Sets full_path if found.
*/
void find_cmd(const char *path, const char *cmd, char full_path[], int *err)
{
    const struct dirent *entry;
    DIR                 *dir_p;
    int                  concat_result;

    dir_p = opendir(path);
    if(dir_p == NULL)
    {
        perror("opendir");
        *err = errno;
        return;
    }
    // readdir_r is deprecated, that's why I suppress
    // cppcheck-suppress readdirCalled
    while((entry = readdir(dir_p)))
    {
        if(strcasecmp(entry->d_name, cmd) == 0)
        {
            concat_result = snprintf(full_path, BUFFER_SIZE, "%s/%s", path, cmd);
            if(concat_result < 0)
            {
                perror("snprintf");
                *err = 1;
                break;
            }
            printf("Found match: %s at %s\n", entry->d_name, full_path);
            // found = 1;
            break;
        }
    }
    closedir(dir_p);
}

/*
    Handles non built-in cmd (cmds that are not cd) by using execv to run cmds.

    full_path: PATH path where executable is located.
    client_argv: array of null-terminated strings, representing the arguments.
    socket_fd: socket to write to.
*/
void handle_nonbuiltin_cmd(const char full_path[], char *client_argv[], int socket_fd, int *err)
{
    pid_t pid;

    pid = fork();
    if(pid == -1)
    {
        perror("fork");
        *err = errno;
        return;
    }

    if(pid == 0)
    {
        // redirect stdout to socket
        dup2(socket_fd, STDOUT_FILENO);
        // close original socket
        close(socket_fd);

        // replace process image with new process with specified args
        execv(full_path, client_argv);

        // if exec fails
        perror("execv");
        exit(1);
    }
}

void clear_array(char *client_argv[])
{
    for(int i = 0; i < MAX_ARGS; i++)
    {
        client_argv[i] = NULL;
    }
}

/*
    Validates command line arguments.
    Returns 0 if ok, 1 if invalid.

*/
// cppcheck-suppress constParameter
void handle_builtin_cmd(char full_path[], char *client_argv[], char message[], int *err)
{
    // count args
    int count = 0;
    while(client_argv[count] != NULL)
    {
        ++count;
    }
    if(count == 0)
    {
        return;
    }

    // no need check for pwd, exit, or echo
    if(strcasecmp(client_argv[0], "cd") == 0)
    {
        // cd can only accept one argument
        if(count > 2)
        {
            // strcpy(message, "cd: too many arguments\n");
            snprintf(message, BUFFER_SIZE, "%s\n", "cd: too many arguments\n");
            return;
        }

        // pass in path
        builtin_cd(client_argv[1], err);
        if(*err != 0)
        {
            // strcpy(message, "cd failed: No such file or directory\n");
            snprintf(message, BUFFER_SIZE, "%s\n", "cd failed: No such file or directory");
            *err = 0;
        }
        else
        {
            // strcpy(message, "cd successful: %s\n", client_argv[1]);
            snprintf(message, BUFFER_SIZE, "cd successful\n");
        }
    }
    else if(strcasecmp(client_argv[0], "pwd") == 0)
    {
        // strcpy(message, getcwd(message, BUFFER_SIZE));
        snprintf(message, BUFFER_SIZE, "%s\n", getcwd(message, BUFFER_SIZE));
    }
    else if(strcasecmp(client_argv[0], "echo") == 0)
    {
        concatenate_argv(client_argv, message);
    }
    else if(strcasecmp(client_argv[0], "type") == 0)
    {
        if(count < 2)
        {
            // strcpy(message, "type: too few arguments\n");
            snprintf(message, BUFFER_SIZE, "%s\n", "type: too few arguments\n");
            return;
        }
        builtin_type(full_path, client_argv, message, err);
    }
}

/*
    Executes builtin cd command.
*/
void builtin_cd(const char *path, int *err)
{
    if(path == NULL)
    {
        // change to home directory
        path = getenv("HOME");
        if(path == NULL)
        {
            perror("HOME not set");
            *err = 1;
            return;
        }
    }

    if(chdir(path) != 0)
    {
        perror("chdir");
        *err = 1;
    }
}

void concatenate_argv(char *client_argv[], char message[])
{
    int position = 0;

    for(int i = 0; i < MAX_ARGS && client_argv[i] != NULL; i++)
    {
        for(int j = 0; client_argv[i][j] != '\0'; j++)
        {
            message[position++] = client_argv[i][j];
        }

        if(client_argv[i + 1] != NULL)
        {    // Add space between arguments
            message[position++] = ' ';
        }
    }
}

void builtin_type(char full_path[], char *client_argv[], char message[], int *err)
{
    char temp_message[BUFFER_SIZE];
    int  count = 1;
    while(client_argv[count] != NULL)
    {
        ++count;
    }
    for(int i = 1; i < count; i++)
    {
        int is_builtin;
        memset(full_path, 0, BUFFER_SIZE);
        memset(temp_message, 0, BUFFER_SIZE);

        is_builtin = is_builtin_cmd(client_argv[i], full_path, err);

        if(is_builtin == 1)
        {
            snprintf(temp_message, BUFFER_SIZE, "%s is a shell builtin\n", client_argv[i]);
        }
        else if(full_path[0] != 0)
        {
            snprintf(temp_message, BUFFER_SIZE, "%s is %s\n", client_argv[i], full_path);
        }
        else if(is_builtin == -1)
        {
            snprintf(temp_message, BUFFER_SIZE, "%s: command not found\n", client_argv[i]);
        }

        // concat the temp_message to the main message
        strncat(message, temp_message, BUFFER_SIZE - strlen(message) - 1);    // -1 to prevent buff overflow
    }
}
