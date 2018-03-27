#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>

#define     QLEN            5
#define     BUFSIZE         4096
#define     THREADS         100

int passivesock(char *service, char *protocol, int qlen, int *rport);

void * handle_client(void * arg) {
    char buf[BUFSIZE];
    int cc;
    int ssock = (int)arg;
    for (;;) {
        if ((cc = (int) read(ssock, buf, BUFSIZE)) <= 0) {
            printf("The client has gone.\n");
            close(ssock);
            break;
        } else {
            buf[cc] = '\0';
            printf("The client says: %s\n", buf);
            if (write(ssock, buf, (size_t) cc) < 0) {
                /* This guy is dead */
                close(ssock);
                break;
            }
        }
    }
}

/*
**	This poor server ... only serves one client at a time
*/
int main(int argc, char *argv[]) {
    char *service;
    struct sockaddr_in fsin;
    int alen;
    int msock;
    int rport = 0;
    pthread_t threads[THREADS];
    int last_thread = 0;

    switch (argc) {
        case 1:
            // No args? let the OS choose a port and tell the user
            rport = 1;
            break;
        case 2:
            // User provides a port? then use it
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
        printf("There was error opening server");
        return 0;
    }

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

        /* start working for this guy */
        /* ECHO what the client says */
        pthread_create(&threads[last_thread++], NULL, handle_client, (void *) ssock);
    }
}


