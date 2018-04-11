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

void *hub(void *args) {
    quiz_group *current_group = all_groups;
    printf("Hub started!\n");
    fflush(stdout);
    for (;;) {
        printf("There are %d clients in hub:\n", current_group->current_size);
        for (int i = 0; i < current_group->current_size; i++) {
            printf("%d-%s\n", current_group->members[i]->sock, current_group->members[i]->name);
        }
        fflush(stdout);
        sleep(1);
    }
    printf("Hub ended\n");
    fflush(stdout);
    pthread_exit(NULL);
}

void *new_client(void *args) {
    int sock = (int) args;
    printf("new thread here, %d has come!\n", sock);
    fflush(stdout);
    client *cl = create_member(sock);
    if (cl == NULL) {
        printf("Couldn't create a client for %d\n", sock);
    } else {
        int status = add_member(0, cl);
        if (status == 0) {
            printf("Client %d joined hub\n", sock);
        }
        if (status == 1) {
            printf("There is no such group\n");
        }
        if (status == 2) {
            printf("Group is already full\n");
        }
        if (status == 3) {
            printf("Client is already in that group\n");
        }
        fflush(stdout);
    }
    pthread_exit(NULL);
}

client * create_member(int sock) {
    client * current_client = all_clients + last_client;
    current_client->id = last_client++;
    current_client->group = NULL;
    current_client->name = NULL;
    current_client->n_len = 0;
    current_client->point = 0;
    current_client->sock = sock;
    return current_client;
}

int add_member(int group_id, client *cl) {
    if (group_id < 0 || group_id > MAX_GROUP_NUM)
        return 1;
    quiz_group *current_group = all_groups + group_id;
    pthread_mutex_lock(groups_mutex + group_id);
    if (current_group->dedicated_size <= 0) {
        pthread_mutex_unlock(groups_mutex + group_id);
        return 1;
    }
    if (current_group->current_size == current_group->dedicated_size) {
        return 2;
    }
    for (int i = 0; i < current_group->current_size; i++) {
        if (current_group->members[i]->id == cl->id)
            return 3;
    }
    current_group->members[current_group->current_size] = cl;
    current_group->current_size++;
    return 0;
}

void init() {
    all_groups->id = 0;
    all_groups->dedicated_size = MAX_CLIENT_NUM;
    all_groups->members = calloc(MAX_CLIENT_NUM, sizeof(client *));
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

    init();

    pthread_create(all_threads + (last_thread++), NULL, hub, NULL);

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
