/*
**	This program is a very simple shell that only handles
**	single word commands (no arguments).
**	Type "quit" to quit.

**  Alibek.manabayev@gmail.com
**  Copyright 2018. All rights reserved.
*/
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CMDLEN 80
#define ARGCNT 10
#define BUFFLEN 2048
#define SEPARATOR " "
#define bool _Bool
#define true 1
#define false 0

struct command_t {
    char *args[ARGCNT];
    int argc;
    int in_fd;
    int out_fd;
};

typedef struct command_t command;

command *create_command();

void parseArguments(char *buff, command *cmd);

void d_log(char *str);

void d_log2(char *str);

void child_proc_background(command *cmd);

void child_proc(command *cmd);

bool checkRedOut_a(command *cmd);

bool checkRedOut(command *cmd);

bool checkRedIn(command *cmd);

bool checkBackground(command *cmd);

void normalize_cmd(command *cmd);

void free_cmd(command *cmd);

bool run_pipe(command *cmd);

void run_command(command *cmd, bool background);

int b_fd = -1;

int main(int argc, char **argv) {
    if (argc > 1) {
        b_fd = open(argv[1], O_RDWR | O_CREAT | O_TRUNC, 0644);
        if (b_fd < 0) {
            puts("Error opening backup file");
            exit(1);
        }
        printf("Backing up to file: %s\n", argv[1]);
    }

    char buff[CMDLEN];
    bool background = false;
    command *cur_cmd;

    d_log("Program begins.\n");

    for (;;) {
        d_log("Please enter a command:   ");
        fflush(stdout);
        fgets(buff, CMDLEN, stdin);
        d_log2(buff);
        buff[strlen(buff) - 1] = '\0';

        // printf("|%s|%d\n", buff, strcmp(buff, "quit"));

        if (strcmp(buff, "quit") == 0)
            break;

        cur_cmd = create_command();
        parseArguments(buff, cur_cmd);

        background = checkBackground(cur_cmd);

        for (int x = 0; x < 3; x++) {
            checkRedIn(cur_cmd);
            checkRedOut(cur_cmd);
            checkRedOut_a(cur_cmd);
        }
        normalize_cmd(cur_cmd);
        // printf("|%d|%d|%d|\n", in_fd, out_fd, err_fd);
        run_command(cur_cmd, background);
        free_cmd(cur_cmd);
    }
}

command *create_command() {
    command *p = malloc(sizeof(command));
    p->argc = 0;
    p->in_fd = -1;
    p->out_fd = -1;
    return p;
}

void normalize_cmd(command *cmd) {
    if (cmd->in_fd == -1) {
        cmd->in_fd = dup(0);
    }
    if (cmd->out_fd == -1) {
        cmd->out_fd = dup(1);
    }
}

void free_cmd(command *cmd) {
    close(cmd->in_fd);
    close(cmd->out_fd);
    free(cmd);
}

void parseArguments(char *buff, command *cmd) {
    int i = 0;
    cmd->args[i] = strtok(buff, SEPARATOR);
    while (cmd->args[i] != NULL) {
        cmd->args[++i] = strtok(NULL, SEPARATOR);
    }
    cmd->argc = i;
}

bool checkRedIn(command *cmd) {
    int n = cmd->argc;
    if (n < 3) {
        return 0;
    }
    if (strcmp(cmd->args[n - 2], "<") != 0) {
        return 0;
    }
    cmd->in_fd = open(cmd->args[n - 1], O_RDONLY);
    if (cmd->in_fd < 0) {
        d_log("Error in opening file.\n");
        exit(1);
    }
    cmd->args[n - 1] = NULL;
    cmd->args[n - 2] = NULL;
    cmd->argc -= 2;
    return 1;
}

bool checkRedOut(command *cmd) {
    int n = cmd->argc;
    if (n < 3) {
        return 0;
    }
    if (strcmp(cmd->args[n - 2], ">") != 0) {
        return 0;
    }
    cmd->out_fd = open(cmd->args[n - 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (cmd->out_fd < 0) {
        d_log("Error in opening file.\n");
        exit(1);
    }
    cmd->args[n - 1] = NULL;
    cmd->args[n - 2] = NULL;
    cmd->argc -= 2;
    return 1;
}

bool checkRedOut_a(command *cmd) {
    int n = cmd->argc;
    if (n < 3) {
        return 0;
    }
    if (strcmp(cmd->args[n - 2], ">>") != 0) {
        return 0;
    }
    cmd->out_fd = open(cmd->args[n - 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (cmd->out_fd < 0) {
        d_log("Error in opening file.\n");
        exit(1);
    }
    cmd->args[n - 1] = NULL;
    cmd->args[n - 2] = NULL;
    cmd->argc -= 2;
    return 1;
}

bool checkBackground(command *cmd) {
    int n = cmd->argc;
    if (n < 2) {
        return false;
    }
    if (strcmp(cmd->args[n - 1], "&") != 0) {
        return false;
    }
    cmd->args[n - 1] = NULL;
    (cmd->argc)--;
    return true;
}

void tee(int in, int out1, int out2) {
    char BUFF[BUFFLEN];
    int n = 0, offset = 0;
    // fputs("-- take a cop of tee --", stderr);
    while ((n = (int) read(in, BUFF, BUFFLEN)) > 0) {
        // fprintf(stderr, "|>%s(%d)<|\n", BUFF, n);
        if (out1 >= 0) {
            offset = (int) write(out1, BUFF, (size_t) n);
            if (offset < 0) {
                d_log("error teeing to out1\n");
                exit(1);
            }
        }
        if (out2 >= 0) {
            offset = (int) write(out2, BUFF, (size_t) n);
            if (offset < 0) {
                d_log("error teeing to out2\n");
                exit(1);
            }
        }
    }
    // fputs("-- enough --", stderr);
}

command *split_commands(command *cmd) {
    int x = -1;
    for (int i = 0; i < cmd->argc; i++) {
        if (strcmp(cmd->args[i], "|") == 0) {
            x = i;
            break;
        }
    }
//    fprintf(stderr, "|%s(%d)|>%d<\n", cmd->args[0], cmd->argc, x);
    if (x == -1) {
        return NULL;
    }
    command *rm = create_command();
    int y = 0;
    for (int i = x + 1; i < cmd->argc; i++) {
        rm->args[y++] = cmd->args[i];
        cmd->args[i] = NULL;
    }
    rm->argc = y;
    cmd->argc = x;
    rm->args[y] = NULL;
    cmd->args[x] = NULL;
    return rm;
}

void run_command(command *cmd, bool background) {
    if (run_pipe(cmd))
        return;
    if (background)
        child_proc_background(cmd);
    else
        child_proc(cmd);
}

bool run_pipe(command *cmd) {
    int total_in = cmd->in_fd;
    int total_out = cmd->out_fd;

    command *rest = split_commands(cmd);
    if (rest == NULL) {
        return false;
    }
//    fprintf(stderr, "pipe: %d<->%d\n", total_in, total_out);
//    fflush(stderr);
    int child_pid;
    int status;
    int fds[2];

    pipe(fds);
    child_pid = fork();
    if (child_pid < 0) {
        d_log("Error in fork.\n");
        exit(1);
    }
    if (child_pid == 0) {
        /* Child process closes up input side of pipe */
        close(fds[0]);
        cmd->in_fd = total_in;
        cmd->out_fd = fds[1];
        run_command(cmd, false);
        exit(0);
    } else {
        /* Parent process closes up output side of pipe */
        close(fds[1]);

        waitpid(child_pid, &status, 0);

        /* Read in a string from the pipe (fds[0])*/
        rest->in_fd = fds[0];
        rest->out_fd = total_out;
        run_command(rest, false);
        close(fds[0]);
    }
    return true;
}

void child_proc(command *cmd) {
//    fprintf(stderr, "child:%d<->%d\n", cmd->in_fd, cmd->out_fd);
    int child_pid;
    int status;
    int fds[2];

    pipe(fds);

    child_pid = fork();

    if (child_pid < 0) {
        d_log("Error in fork.\n");
        exit(1);
    }

    if (child_pid != 0) {
        /* Parent process closes up output side of pipe */
        close(fds[1]);

        /* Read in a string from the pipe (fds[0])*/
        tee(fds[0], cmd->out_fd, b_fd);
        waitpid(child_pid, &status, 0);
        close(fds[0]);
    } else {
        /* Child process closes up input side of pipe */
        close(fds[0]);
        dup2(cmd->in_fd, 0);
        dup2(fds[1], 1);
        execvp(cmd->args[0], cmd->args);

        exit(0);
    }
}

void child_proc_background(command *cmd) {
    int childpid;

    childpid = fork();

    if (childpid < 0) {
        d_log("Error in fork.\n");
        exit(1);
    }

    if (childpid != 0) {
        return;
    } else {
        child_proc(cmd);
        exit(0);
    }
}

void d_log(char *str) {
    printf("%s", str);
    if (b_fd != -1) {
        d_log2(str);
    }
}

void d_log2(char *str) { write(b_fd, str, strlen(str)); }
