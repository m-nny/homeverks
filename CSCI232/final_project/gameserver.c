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
    q_group *c_group = all_groups;
    fd_set set;
    struct timeval timeout;
    int mx, cc;
    char *buffer = calloc(MAX_BUFFER_SIZE, sizeof(char));
    printf("Hub started!\n");
    fflush(stdout);
    for (;;) {
        pthread_mutex_lock(g_mutex);
        FD_ZERO(&set);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        mx = 0;
        for (int i = 0; i < c_group->c_size; i++) {
            client *c_client = c_group->members[i];
            FD_SET(c_client->sock, &set);
            if (c_client->sock > mx)
                mx = c_client->sock;
        }
        pthread_mutex_unlock(g_mutex);

        printf("round again. %d\n", mx);
        select(mx + 1, &set, NULL, NULL, &timeout);

        pthread_mutex_lock(g_mutex);
        for (int i = 0; i < c_group->c_size; i++) {
            client *c_client = c_group->members[i];
            if (FD_ISSET(c_client->sock, &set)) {
                cc = (int) read(c_client->sock, buffer, MAX_BUFFER_SIZE);
                if (cc <= 0) {
                    remove_member(0, c_client);
                    continue;
                }
                cc = normalize(buffer, cc);
                printf("%d says:|%s|\n", c_client->sock, buffer);
            }
        }
        fflush(stdout);
        pthread_mutex_unlock(g_mutex);
    }
}

void *new_client(void *args) {
    int sock = (int) args;
    printf("new thread here, %d has come!\n", sock);
    fflush(stdout);
    client *cl = create_member(sock);
    if (cl == NULL) {
        printf("Couldn't create a client for %d\n", sock);
        pthread_exit(NULL);
    }
    char *msg_to_send = NULL;
    int m_len;
    m_len = open_groups(&msg_to_send);
    int cc = (int) write(sock, msg_to_send, (size_t) m_len);
    if (cc <= 0) {
        printf("Client %d has gone\n", sock);
        pthread_exit(NULL);
    }
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

    pthread_exit(NULL);
}

client *create_member(int sock) {
    client *current_client = all_clients + l_client;
    current_client->id = l_client++;
    current_client->group = NULL;
    current_client->name = NULL;
    current_client->n_len = 0;
    current_client->point = 0;
    current_client->sock = sock;
    return current_client;
}

int add_member(int g_id, client *cl) {
    if (g_id < 0 || g_id > MAX_GROUP_NUM)
        return 1;
    q_group *c_group = all_groups + g_id;
    pthread_mutex_lock(g_mutex + g_id);
    if (c_group->d_size <= 0) {
        pthread_mutex_unlock(g_mutex + g_id);
        return 1;
    }
    if (c_group->c_size == c_group->d_size) {
        pthread_mutex_unlock(g_mutex + g_id);
        return 2;
    }
    for (int i = 0; i < c_group->c_size; i++) {
        if (c_group->members[i]->id == cl->id) {
            pthread_mutex_unlock(g_mutex + g_id);
            return 3;
        }
    }
    c_group->members[c_group->c_size] = cl;
    c_group->c_size++;
    pthread_mutex_unlock(g_mutex + g_id);
    return 0;
}

int remove_member(int g_id, client *cl) {
    if (g_id < 0 || g_id > MAX_GROUP_NUM)
        return 1;
    q_group *c_group = all_groups + g_id;
    pthread_mutex_lock(g_mutex + g_id);
    if (c_group->d_size <= 0) {
        pthread_mutex_unlock(g_mutex + g_id);
        return 1;
    }
    for (int i = 0; i < c_group->c_size; i++) {
        if (c_group->members[i]->id == cl->id) {
            c_group->members[i] = NULL;
            c_group->c_size--;
            pthread_mutex_unlock(g_mutex + g_id);
            return 0;
        }
    }
    pthread_mutex_unlock(g_mutex + g_id);
    return 2;
}

int open_groups(char **p_str) {
    char *str = calloc(MAX_BUFFER_SIZE, sizeof(char));
    int offset = 0;
    offset = sprintf(str, "OPENGROUPS");
    for (int i = 1; i < l_group; i++) {
        q_group *current_group = all_groups + i;
        pthread_mutex_lock(g_mutex + i);
        if (current_group->d_size > 0)
            offset += sprintf(str + offset, "|%s|%s|%d|%d", current_group->topic, current_group->name,
                              current_group->d_size, current_group->c_size);
        pthread_mutex_unlock(g_mutex + i);
    }
    offset += sprintf(str + offset, "\r\n");
    *p_str = str;
    return offset;
}

void init() {
    all_groups->id = 0;
    all_groups->d_size = MAX_CLIENT_NUM;
    all_groups->members = calloc(MAX_CLIENT_NUM, sizeof(client *));
    l_group++;
    pthread_create(all_threads + (l_thread++), NULL, hub, NULL);

    /// TESTING ONLY
    (all_groups + l_group)->id = 1;
    (all_groups + l_group)->d_size = 2;
    (all_groups + l_group)->members = calloc(2, sizeof(client *));
    l_group++;
    /// TESTING ONLY
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
}
