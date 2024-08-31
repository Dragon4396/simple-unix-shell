#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <pwd.h>

#define COMMAND_LENGTH 1024
#define NUM_TOKENS (COMMAND_LENGTH / 2 + 1)
#define HISTORY_DEPTH 10

char history[HISTORY_DEPTH][COMMAND_LENGTH];
int history_count = 0;
bool is_builtin_command = false;
char prev_dir[COMMAND_LENGTH] = "";

int tokenize_command(char *buff, char *tokens[])
{
    int token_count = 0;
    _Bool in_token = false;
    int num_chars = strnlen(buff, COMMAND_LENGTH);
    for (int i = 0; i < num_chars; i++)
    {
        switch (buff[i])
        {
        case ' ':
        case '\t':
        case '\n':
            buff[i] = '\0';
            in_token = false;
            break;
        default:
            if (!in_token)
            {
                tokens[token_count] = &buff[i];
                token_count++;
                in_token = true;
            }
        }
    }
    tokens[token_count] = NULL;
    return token_count;
}

void read_history(char *buff)
{
    if (buff[0] != '!' && strcmp(buff, "!-") != 0)
    {
        strncpy(history[history_count % HISTORY_DEPTH], buff, COMMAND_LENGTH);
        history_count++;
    }
}

void print_history(int count)
{
    int start = (history_count > HISTORY_DEPTH) ? history_count - HISTORY_DEPTH : 0;
    for (int i = history_count - 1; i >= start; i--)
    {
        char temp[10];
        sprintf(temp, "%d", i);
        write(STDOUT_FILENO, temp, strlen(temp));
        write(STDOUT_FILENO, "\t", strlen("\t"));
        write(STDOUT_FILENO, history[i % HISTORY_DEPTH], strlen(history[i % HISTORY_DEPTH]));
        write(STDOUT_FILENO, "\n", strlen("\n"));
    }
}

void print_help()
{
    char *help_message = "exit: Exit the shell.\n"
                         "pwd: Print the current working directory.\n"
                         "cd: Change the current working directory.\n"
                         "help: Display this help message.\n"
                         "history: Show command history.\n";

    write(STDOUT_FILENO, help_message, strlen(help_message));
}

void handle_SIGINT()
{
    is_builtin_command = true;
    write(STDOUT_FILENO, "\n", strlen("\n"));
    print_history(HISTORY_DEPTH);
}

void clear_history()
{
    memset(history, 0, sizeof(history));
    history_count = 0;
}

void read_command(char *buff, char *tokens[], _Bool *in_background)
{
    *in_background = false;

    // Read input
    int length = read(STDIN_FILENO, buff, COMMAND_LENGTH - 1);
    if ((length < 0) && (errno != EINTR))
    {
        perror("Unable to read command. Terminating.\n");
        exit(-1); /* terminate with error */
    }

    // Null terminate and strip \n.
    buff[length] = '\0';
    if (buff[strlen(buff) - 1] == '\n')
    {
        buff[strlen(buff) - 1] = '\0';
    }

    read_history(buff);

    // Tokenize (saving original command string)
    int token_count = tokenize_command(buff, tokens);
    if (token_count == 0)
    {
        return;
    }

    // Extract if running in background:
    if (token_count > 0 && strcmp(tokens[token_count - 1], "&") == 0)
    {
        *in_background = true;
        tokens[token_count - 1] = NULL;
    }
}

int main(int argc, char *argv[])
{
    char input_buffer[COMMAND_LENGTH];
    char *tokens[NUM_TOKENS];
    char cwd[COMMAND_LENGTH];
    char prompt[COMMAND_LENGTH + 3];
    if (chdir(getenv("HOME")) != 0)
    {
        perror("Failed to set home directory");
        exit(-1);
    }
    bool is_history = false;
    while (true)
    {
        is_builtin_command = false;
        struct sigaction handler;
        handler.sa_handler = handle_SIGINT;
        handler.sa_flags = 0;
        sigemptyset(&handler.sa_mask);
        sigaction(SIGINT, &handler, NULL);

        if (getcwd(cwd, sizeof(cwd)) == NULL)
        {
            perror("Failed to get current working directory");
            exit(-1);
        }
        strcpy(prompt, cwd);
        strcat(prompt, "$ ");
        
        _Bool in_background = false;
        if (is_history == false)
        {
            write(STDOUT_FILENO, prompt, strlen(prompt));
            read_command(input_buffer, tokens, &in_background);
        }

        if (!is_builtin_command && tokens[0] != NULL)
        {
            if (strcmp(tokens[0], "exit") == 0)
            {
                if (tokens[1] != NULL)
                {
                    write(STDOUT_FILENO, "exit: too many arguments\n", strlen("exit: too many arguments\n"));
                }
                else
                {
                    clear_history();
                    exit(0);
                }
            }
            else if (strcmp(tokens[0], "pwd") == 0)
            {
                getcwd(cwd, sizeof(cwd));
                write(STDOUT_FILENO, cwd, strlen(cwd));
                write(STDOUT_FILENO, "\n", strlen("\n"));
                is_builtin_command = true;
            }
            else if (strcmp(tokens[0], "cd") == 0)
            {
                char *target_dir;
                if (tokens[1] == NULL || strcmp(tokens[1], "~") == 0)
                {
                    target_dir = getenv("HOME");
                }
                else if (strcmp(tokens[1], "-") == 0)
                {
                    target_dir = prev_dir[0] ? prev_dir : getenv("HOME");
                }
                else
                {
                    target_dir = tokens[1];
                }

                if (getcwd(prev_dir, sizeof(prev_dir)) == NULL)
                {
                    perror("Failed to get current working directory");
                }

                if (chdir(target_dir) == -1)
                {
                    write(STDOUT_FILENO, "ERROR: Invalid Directory\n", strlen("ERROR: Invalid Directory\n"));
                }

                is_builtin_command = true;
            }
            else if (strcmp(tokens[0], "help") == 0)
            {
                if (tokens[1] == NULL)
                {
                    print_help();
                }
                else
                {
                    if (strcmp(tokens[1], "exit") == 0)
                    {
                        write(STDOUT_FILENO, "exit: Exit the shell.\n", strlen("exit: Exit the shell.\n"));
                    }
                    else if (strcmp(tokens[1], "pwd") == 0)
                    {
                        write(STDOUT_FILENO, "pwd: Print the current working directory.\n", strlen("pwd: Print the current working directory.\n"));
                    }
                    else if (strcmp(tokens[1], "cd") == 0)
                    {
                        write(STDOUT_FILENO, "cd: Change the current working directory.\n", strlen("cd: Change the current working directory.\n"));
                    }
                    else if (strcmp(tokens[1], "help") == 0)
                    {
                        write(STDOUT_FILENO, "help: Display this help message.\n", strlen("help: Display this help message.\n"));
                    }
                    else if (strcmp(tokens[1], "history") == 0)
                    {
                        write(STDOUT_FILENO, "history: Show command history.\n", strlen("history: Show command history.\n"));
                    }
                }
                is_builtin_command = true;
            }
            else if (strcmp(tokens[0], "history") == 0)
            {
                print_history(HISTORY_DEPTH);
                is_builtin_command = true;
            }
            else if (strcmp(tokens[0], "!!") == 0)
            {
                if (history_count > 0)
                {
                    char last_command[COMMAND_LENGTH];
                    strcpy(last_command, history[(history_count - 1) % HISTORY_DEPTH]);
                    write(STDOUT_FILENO, last_command, strlen(last_command));
                    write(STDOUT_FILENO, "\n", strlen("\n"));
                    read_history(last_command);
                    tokenize_command(last_command, tokens);
                    is_builtin_command = false;
                    is_history = true;
                    continue;
                }
                else
                {
                    write(STDOUT_FILENO, "ERROR: No previous commands in history.\n", strlen("ERROR: No previous commands in history.\n"));
                    is_builtin_command = true;
                }
            }
            else if (strcmp(tokens[0], "!-") == 0)
            {
                clear_history();
                is_builtin_command = true;
            }
            else if (tokens[0][0] == '!' && isdigit(tokens[0][1]))
            {
                int n = atoi(tokens[0] + 1);
                if (n < 0 || n >= history_count)
                {
                    write(STDOUT_FILENO, "ERROR: Invalid command history number.\n", strlen("ERROR: Invalid command history number.\n"));
                }
                else
                {
                    char nth_command[COMMAND_LENGTH];
                    strcpy(nth_command, history[n % HISTORY_DEPTH]);
                    write(STDOUT_FILENO, nth_command, strlen(nth_command));
                    write(STDOUT_FILENO, "\n", strlen("\n"));
                    read_history(nth_command);
                    tokenize_command(nth_command, tokens);
                    is_builtin_command = false;
                    is_history = true;
                    continue;
                }
            }
        }
        if (is_history)
        {
            is_history = false;
        }
        
        if (!is_builtin_command && tokens[0] != NULL)
        {
            pid_t pid = fork();
            if (pid == 0) // Child process
            {
                execvp(tokens[0], tokens);
                perror("execvp Failed");
                exit(1);
            }
            else if (pid > 0) // Parent process
            {
                if (in_background == false)
                {
                    waitpid(pid, NULL, 0);
                }
            }
            else
            {
                 perror("fork Failed");
            }

            while (waitpid(-1, NULL, WNOHANG) > 0)
                ; // do nothing
        }
    }
    return 0;
}