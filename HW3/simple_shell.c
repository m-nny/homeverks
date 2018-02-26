/*
**	This program is a very simple shell that only handles
**	single word commands (no arguments).
**	Type "quit" to quit.

**  Alibek.manabayev@gmail.com
**  Copyright 2018. All rights reserved.
*/
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define CMDLEN 80
#define ARGCNT 10
#define BUFFLEN 2048
#define SEPARATOR " "
#define bool _Bool
#define true 1
#define false 0

int parseArguments(char *buff, char **args);
void d_log(char *str);
void d_log2(char *str);
void child_proc_background(char *arguments[ARGCNT]);
void child_proc(char *arguments[ARGCNT]);
bool checkRedOut_a(char **args, int n);
bool checkRedOut(char **args, int n);
bool checkRedIn(char **args, int n);

int b_fd = -1;
int in_fd = 0;
int out_fd = 1;
int err_fd = 2;

int main(int argc, char **argv) {
  if (argc > 1) {
    b_fd = open(argv[1], O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (b_fd < 0) {
      puts("Error opening backup file");
      exit(1);
    }
    printf("Backing up to file: %s\n", argv[1]);
  }

  int i, fd;
  char buff[CMDLEN];
  char command[CMDLEN];
  char *arguments[ARGCNT];
  bool background = false;

  d_log("Program begins.\n");

  for (;;) {
    d_log("Please enter a command:   ");
    fgets(buff, CMDLEN, stdin);
    d_log2(buff);
    buff[strlen(buff) - 1] = '\0';

    // printf("|%s|%d\n", buff, strcmp(buff, "quit"));

    if (strcmp(buff, "quit") == 0)
      break;

    int n = parseArguments(buff, arguments);

    background = n > 0 && !strcmp(arguments[n - 1], "&");

    if (background) {
      arguments[n - 1] = NULL;
      n--;
    }

    in_fd = dup(0);
    out_fd = dup(1);
    err_fd = dup(2);

    for (int x = 0; x < 3; x++) {
      if (checkRedIn(arguments, n))
        n -= 2;
      if (checkRedOut(arguments, n))
        n -= 2;
      if (checkRedOut_a(arguments, n))
        n -= 2;
    }
    // printf("|%d|%d|%d|\n", in_fd, out_fd, err_fd);
    if (background) {
      child_proc_background(arguments);
    } else {
      child_proc(arguments);
    }
    close(in_fd);
    close(out_fd);
    close(err_fd);
  }
}

int parseArguments(char *buff, char **args) {
  int i = 0;
  args[i] = strtok(buff, SEPARATOR);
  while (args[i] != NULL) {
    args[++i] = strtok(NULL, SEPARATOR);
  }
  return i;
}

bool checkRedIn(char **args, int n) {
  if (n < 3) {
    return 0;
  }
  if (strcmp(args[n - 2], "<") != 0) {
    return 0;
  }
  in_fd = open(args[n - 1], O_RDONLY);
  if (in_fd < 0) {
    d_log("Error in opening file.\n");
    exit(1);
  }
  args[n - 1] = NULL;
  args[n - 2] = NULL;
  return 1;
}

bool checkRedOut(char **args, int n) {
  if (n < 3) {
    return 0;
  }
  if (strcmp(args[n - 2], ">") != 0) {
    return 0;
  }
  out_fd = open(args[n - 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (out_fd < 0) {
    d_log("Error in opening file.\n");
    exit(1);
  }
  args[n - 1] = NULL;
  args[n - 2] = NULL;
  return 1;
}

bool checkRedOut_a(char **args, int n) {
  if (n < 3) {
    return 0;
  }
  if (strcmp(args[n - 2], ">>") != 0) {
    return 0;
  }
  out_fd = open(args[n - 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
  if (in_fd < 0) {
    d_log("Error in opening file.\n");
    exit(1);
  }
  args[n - 1] = NULL;
  args[n - 2] = NULL;
  return 1;
}

void tee(int in, int out1, int out2) {
  char BUFF[BUFFLEN];
  int n = 0, offset = 0;
  // fputs("-- take a cop of tee --", stderr);
  while ((n = read(in, BUFF, BUFFLEN)) > 0) {
    // fprintf(stderr, "|>%s(%d)<|\n", BUFF, n);
    if (out1 >= 0) {
      offset = write(out1, BUFF, n);
      if (offset < 0) {
        d_log("error teeing to out1\n");
        exit(1);
      }
    }
    if (out2 >= 0) {
      offset = write(out2, BUFF, n);
      if (offset < 0) {
        d_log("error teeing to out2\n");
        exit(1);
      }
    }
  }
  // fputs("-- enough --", stderr);
}

void child_proc(char *arguments[ARGCNT]) {
  int childpid;
  int status;
  int fds[2];

  pipe(fds);

  childpid = fork();

  if (childpid < 0) {
    d_log("Error in fork.\n");
    exit(-1);
  }

  if (childpid != 0) {
    /* Parent process closes up output side of pipe */
    close(fds[1]);

    /* Read in a string from the pipe (fds[0])*/
    tee(fds[0], out_fd, b_fd);
    waitpid(childpid, &status, 0);
    close(fds[0]);
  } else {
    /* Child process closes up input side of pipe */
    close(fds[0]);
		dup2(in_fd, 0);
    dup2(fds[1], 1);
    execvp(arguments[0], arguments);

    exit(0);
  }
}

void child_proc_background(char *arguments[ARGCNT]) {
  int childpid;
  int status;

  childpid = fork();

  if (childpid < 0) {
    d_log("Error in fork.\n");
    exit(-1);
  }

  if (childpid != 0) {
    return;
  } else {
    child_proc(arguments);
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
