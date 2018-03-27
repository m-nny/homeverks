#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define     QLEN            5
#define     BUFSIZE         4096
#define     THREADS         100
#define     SEPARATOR       "|"
#define     ARGCNT          5
#define     WELCOMESTR      "QS|ADMIN\r\n"
#define     JOINSTR         "QS|JOIN\r\n"
#define     FULLSTR         "QS|FULL\r\n"
#define     WAITSTR         "WAIT\r\n"
#define     sock_exit       {close(ssock); pthread_exit(NULL);}
#define     sock_assert(x)  if (!(x)) {printf("Some error occurred\n"); sock_exit }

int passivesock(char *service, char *protocol, int qlen, int *rport);

int client_number = 0;
int group_size = 0;
char * client_names[THREADS];
int first_answer = -1;
pthread_mutex_t mutex;
pthread_cond_t cond;

int parseArguments(char * buff, char ** args) {
    int i = 0;
    char * rem;
    args[i] = strtok_r(buff, SEPARATOR, &rem);
    while (args[i] != NULL) {
        args[++i] = strtok_r(NULL, SEPARATOR, &rem);
    }
    return i;
}

int normalize(char * str, int cc) {
    str[cc] = 0;
    if (cc > 0 && str[cc - 1] == '\n')
        str[--cc] = 0;
    if (cc > 0 && str[cc - 1] == '\r')
        str[--cc] = 0;
    if (cc > 0 && str[cc - 1] == '\n')
        str[--cc] = 0;
    return cc;
}

void * handle_client(void * arg) {
    char buf[BUFSIZE];
    char *msg[ARGCNT];
    int cc;
    int ssock = (int)arg;
    int msg_c;
    int status = -1;
    int client_id = -1;

    pthread_mutex_lock(&mutex);
    if (client_number == 0) {
        status = 1;
        sock_assert ((cc = (int) write(ssock, WELCOMESTR, strlen(WELCOMESTR))) > 0);
        sock_assert ((cc = (int) read(ssock, buf, BUFSIZE)) > 0);
        cc = normalize(buf, cc);
        printf("The client says: |[%s]|\n", buf);

        msg_c = parseArguments(buf, msg);
        sock_assert (msg_c == 3 && strcmp(msg[0], "GROUP") == 0);
        printf("[%s(%d), %s(%d), %s(%d)]\n", msg[0], (int) strlen(msg[0]),
               msg[1], (int) strlen(msg[1]), msg[2], (int) strlen(msg[2]));
        sock_assert ((cc = (int) write(ssock, WAITSTR, strlen(WAITSTR))) > 0);

        client_id = client_number++;
        client_names[client_id] = calloc(strlen(msg[1]), sizeof(char));
        strcpy(client_names[client_id], msg[1]);
        group_size = atoi(msg[2]);

        pthread_cond_signal(&cond);
    }
    pthread_mutex_unlock(&mutex);

    if (status == -1) {
        sock_assert ((cc = (int) write(ssock, JOINSTR, strlen(JOINSTR))) > 0);
        sock_assert ((cc = (int) read(ssock, buf, BUFSIZE)) > 0);
        cc = normalize(buf, cc);
        printf("The client says: |[%s]|\n", buf);
        fflush(stdout);

        msg_c = parseArguments(buf, msg);
        sock_assert (msg_c == 2 && strcmp(msg[0], "JOIN") == 0);
        printf("[%s(%d), %s(%d)]\n", msg[0], (int) strlen(msg[0]), msg[1], (int) strlen(msg[1]));
        fflush(stdout);

        sock_assert ((cc = (int) write(ssock, WAITSTR, strlen(WAITSTR))) > 0);

        pthread_mutex_lock(&mutex);
        client_id = client_number++;
        client_names[client_id] = calloc(strlen(msg[1]), sizeof(char));
        strcpy(client_names[client_id], msg[1]);

        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }
    printf("%s has joined.\n", client_names[client_id]);
    printf("%d out of %d\n", client_id, group_size);
    fflush(stdout);

    while (client_number < group_size) {
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond, &mutex);
        pthread_mutex_unlock(&mutex);
    }
    printf("[%s(%d), %s(%d)]\n", msg[0], (int) strlen(msg[0]), msg[1], (int) strlen(msg[1]));
    sock_assert ((cc = (int) write(ssock, FULLSTR, strlen(FULLSTR))) > 0);

    sock_assert ((cc = (int) read(ssock, buf, BUFSIZE)) > 0);
    cc = normalize(buf, cc);
    printf("The client says: |[%s]|\n", buf);
    fflush(stdout);

    status = -1;
    pthread_mutex_lock(&mutex);
    if (first_answer == -1) {
        status = 1;
        first_answer = client_id;
        sprintf(buf, "Congratulations!!!\nYou answered first.\r\n");
        sock_assert ((cc = (int) write(ssock, buf, strlen(buf))) > 0);
        printf("%s won this game\n", client_names[first_answer]);
        fflush(stdout);
    }
    pthread_mutex_unlock(&mutex);
    if (status == -1) {
        sprintf(buf, "Sorry, too late!!!\n%s were the one, who answered first.\r\n", client_names[first_answer]);
        sock_assert ((cc = (int) write(ssock, buf, strlen(buf))) > 0);
    }
    sock_exit;
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
    pthread_mutex_init(&mutex, NULL);
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
    pthread_mutex_destroy(&mutex);
    pthread_exit(NULL);
}


