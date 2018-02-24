/*
**	This program is a very simple shell that only handles
**	single word commands (no arguments).
**	Type "quit" to quit.
*/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define CMDLEN 80
#define ARGCNT 10
#define SEPARATOR " "
#define bool _Bool
#define true 1
#define false 0

int parseArguments(char *buff, char **args) {
  int i = 0;

  args[i] = strtok(buff, SEPARATOR);

  while (args[i] != NULL) {
    // printf("%s\n", args[i]);
    args[++i] = strtok(NULL, SEPARATOR);
  }

  // printf("%d\n", i);
  return i;
}

int main() {
  int   pid;
  int   status;
  int   i, fd;
  char  buff[CMDLEN];
  char  command[CMDLEN];
  char *arguments[ARGCNT];
  bool  background = false;

  printf("Program begins.\n");

  for (;;) {
    printf("Please enter a command:   ");
    fgets(buff, CMDLEN, stdin);
    buff[strlen(buff) - 1] = '\0';

    // printf("|%s|%d\n", buff, strcmp(buff, "quit"));

    if (strcmp(buff, "quit") == 0) break;

    int n = parseArguments(buff, arguments);

    for (int i = 0; arguments[i] != NULL; i++) {
      printf("\\%s/\n", arguments[i]);
    }

    background = n > 0 && !strcmp(arguments[n - 1], "&");

    if (background) {
      arguments[n - 1] = NULL;
    }

    pid = fork();

    if (pid < 0) {
      printf("Error in fork.\n");
      exit(-1);
    }

    if (pid != 0) {
      if (!background) {
        waitpid(pid, &status, 0);
      }
    } else {
      execvp(arguments[0], arguments);
      break;
    }
  }
}
