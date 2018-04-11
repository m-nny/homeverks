#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "gameserver.h"

int passivesock(char *service, char *protocol, int qlen, int *rport);

int parseArguments(char *buff, char **args) {
    int i = 0;
    char *rem;
    args[i] = strtok_r(buff, SEPARATOR, &rem);
    while (args[i] != NULL) {
        args[++i] = strtok_r(NULL, SEPARATOR, &rem);
    }
    return i;
}

int normalize(char *str, int cc) {
    str[cc] = 0;
    if (cc > 0 && str[cc - 1] == '\n')
        str[--cc] = 0;
    if (cc > 0 && str[cc - 1] == '\r')
        str[--cc] = 0;
    if (cc > 0 && str[cc - 1] == '\n')
        str[--cc] = 0;
    return cc;
}

void * hub(void * args) {
    printf("Hub started!\n");
    fflush(stdout);
    for (int i = 0; i < 10; i++) {
        sleep(1);
        printf("Hub is awaken\n");
    }
    printf("Hub ended\n");
    fflush(stdout);
    pthread_exit(NULL);
}

void * new_client(void * args) {
    int sock = (int)args;
    printf("new thread here, %d has come!\n", sock);
    fflush(stdout);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    char *service = NULL;
    struct sockaddr_in fsin;
    int alen;
    int msock;
    int rport = 0;
    switch (argc) {
        case 1:
            // one arg? let the OS choose a port and tell the user
            //filename = argv[1];
            rport = 1;
            break;
        case 2:
            // User provides a port? then use it
            //filename = argv[1];
            service = argv[1];
            break;
        default:
            fprintf(stderr, "usage: server [port]\n");
            exit(-1);
    }

    msock = passivesock(service, "tcp", QLEN, &rport);
    if (rport) {
        //	Tell the user the selected port
        printf("server: port %d\n", rport);
        fflush(stdout);
    } else {
        printf("server: port %s\n", service);
        fflush(stdout);
    }

    if (msock < 0) {
        fprintf(stderr, "There was error opening server\n");
        return 0;
    }

    pthread_create(&all_threads[last_thread++], NULL, hub, NULL);

    for (;;) {
        int ssock;

        alen = sizeof(fsin);
        ssock = accept(msock, (struct sockaddr *) &fsin, (socklen_t *) &alen);
        if (ssock < 0) {
            fprintf(stderr, "accept: %s\n", strerror(errno));
            exit(-1);
        }

        printf("A client has arrived for echoes.\n");
        fflush(stdout);
        pthread_t temp_pthread;
        pthread_create(&temp_pthread, NULL, new_client, (void *) ssock);
    }
    pthread_exit(NULL);
}
