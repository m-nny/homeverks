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

int parse_args(char *str, char **args) {
    int i = 0;
    char *rem;
    args[i] = strtok_r(str, SEPARATOR, &rem);
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

void set_string(char *origin, char **dest, int *len) {
    *len = (int) strlen(origin);
    *dest = calloc((size_t) (*len) + 1, sizeof(char));
    strcpy(*dest, origin);
}

client_p create_client(int sock) {
    client_p client = all_clients + l_client;
    client->id = l_client++;
    client->g_id = -1;
    free(client->name);
    client->name = NULL;
    client->n_len = 0;
    client->point = 0;
    client->sock = sock;
    return client;
}

int destroy_client(client_p client) {
    remove_member(client->g_id, client);
    close(client->sock);
    free(client->name);
    client->name = NULL;
    return 0;
}

group_p create_group(int id, int dedicated_size) {
    ///TO CHANGE
    id = l_group;
    group_p group = all_groups + (l_group++);
    group->c_size = 0;
    group->d_size = dedicated_size;
    free(group->members);
    group->members = calloc((size_t) dedicated_size, sizeof(client_p));
    group->id = l_group;
    group->admin = NULL;
    free(group->name);
    group->name = NULL;
    group->n_len = -1;
    free(group->topic);
    group->topic = NULL;
    group->t_len = -1;
    pthread_mutex_init(g_mutex + id, NULL);
    return group;
}

int destroy_group(int id) {
    if (id < 0 || id > MAX_GROUP_NUM)
        return 1;
    group_p group = all_groups + id;

    free(group->members);
    group->members = NULL;

    free(group->topic);
    group->topic = NULL;

    free(group->name);
    group->name = NULL;
    pthread_mutex_destroy(g_mutex + id);
    return 0;
}

int add_member(int g_id, client_p client) {
    if (g_id < 0 || g_id > MAX_GROUP_NUM)
        return 1;
    group_p group = all_groups + g_id;
    if (group->d_size <= 0) {
        return 1;
    }
    if (group->c_size == group->d_size) {
        return 2;
    }
    for (int i = 0; i < group->c_size; i++) {
        if (group->members[i]->id == client->id) {
            return 3;
        }
    }
    group->members[group->c_size] = client;
    group->c_size++;
    client->g_id = g_id;
    return 0;
}

int remove_member(int g_id, client_p client) {
    if (g_id < 0 || g_id > MAX_GROUP_NUM)
        return 1;
    group_p group = all_groups + g_id;
    if (group->d_size <= 0) {
        return 1;
    }
    for (int i = 0; i < group->c_size; i++) {
        if (group->members[i]->id == client->id) {
            group->members[i] = NULL;
            group->c_size--;
            return 0;
        }
    }
    return 2;
}

int open_groups(char **p_str) {
    char *str = calloc(MAX_BUFFER_SIZE, sizeof(char));
    int offset = 0;
    offset = sprintf(str, "OPENGROUPS");
    for (int i = 1; i < l_group; i++) {
        group_p group = all_groups + i;
        pthread_mutex_lock(g_mutex + i);
        if (group->d_size > 0)
            offset += sprintf(str + offset, "|%s|%s|%d|%d", group->topic, group->name,
                              group->d_size, group->c_size);
        pthread_mutex_unlock(g_mutex + i);
    }
    offset += sprintf(str + offset, "\r\n");
    *p_str = str;
    return offset;
}

int find_group(char *name) {
    for (int i = 1; i < l_group; i++) {
        group_p group = all_groups + i;
        if (group->name != NULL && strcmp(name, group->name) == 0) {
            return i;
        }
    }
    return -1;
}

int read_quiz(client_p client) {
    group_p group = all_groups + client->g_id;
    char *buffer = calloc(MAX_QUEST_NUM * MAX_QUEST_LEN + 10, sizeof(char));
    int b_len = MAX_QUEST_NUM * MAX_QUEST_LEN + 9;
    int offset = 0;
    int need_size = b_len;
    int cc;
    do {
        cc = (int) read(client->sock, buffer + offset, (size_t) need_size);
        if (cc <= 0) {
            return -1;
        }
        offset += cc;
        need_size -= cc;
    } while (need_size > 0);
    return 1;
}

void init() {
    group_p hub_group = create_group(0, MAX_CLIENT_NUM);
    pthread_create(all_threads + (l_thread++), NULL, hub, NULL);

    /// TESTING ONLY
    group_p test_group = create_group(1, 2);
    test_group->topic = calloc(12, sizeof(char));
    strcpy(test_group->topic, "some_topic");
    test_group->name = calloc(12, sizeof(char));
    strcpy(test_group->name, "some_name");
    /// TESTING ONLY
}

void handle_join(char **tokens, int cc, client_p client) {
    if (cc != 3) {
        write(client->sock, "BAD|Wrong format\r\n", 18);
        return;
    }
    remove_member(0, client);
    int g_id = find_group(tokens[1]);
    if (g_id < 0) {
        cc = (int) write(client->sock, "BAD|No such group\r\n", 19);
        if (cc <= 0) {
            destroy_client(client);
        }
        return;
    }
    set_string(tokens[2], &(client->name), &(client->n_len));
    int status = add_member(g_id, client);
    switch (status) {
        case 0:
            cc = (int) write(client->sock, "OK\r\n", 4);
            break;
        case 1:
            cc = (int) write(client->sock, "BAD|Group is full\r\n", 19);
            break;
        case 2:
            cc = (int) write(client->sock, "BAD|Already there\r\n", 19);
            break;
        default:
            break;
    }
    if (cc <= 0) {
        destroy_client(client);
    }
}

void handle_group(char **tokens, int cc, client_p client) {
    if (cc != 4) {
        cc = (int) write(client->sock, "BAD|Wrong format\r\n", 18);
        if (cc <= 0) {
            destroy_client(client);
        }
        return;
    }
    char *e_ptr;
    int g_size = (int) strtol(tokens[3], &e_ptr, 10);
    if (tokens[3] == e_ptr) {
        cc = (int) write(client->sock, "BAD|Wrong format\r\n", 18);
        if (cc <= 0) {
            destroy_client(client);
        }
        return;
    }
    group_p c_group = create_group(-1, g_size);
    set_string(tokens[1], &(c_group->topic), &(c_group->t_len));
    set_string(tokens[2], &(c_group->name), &(c_group->n_len));

    cc = (int) write(client->sock, "SENDQUIZ\r\n", 10);
    if (cc <= 0) {
        destroy_client(client);
    }
    return;
    ///TODO
    cc = read_quiz(client);
    if (cc == -2) {
        cc = (int) write(client->sock, "BAD|Wrong format\r\n", 18);
        if (cc <= 0) {
            destroy_client(client);
        }
        return;
    }
    if (cc <= 0) {
        destroy_client(client);
    }

    pthread_mutex_lock(g_mutex);
    remove_member(0, client);
    pthread_mutex_unlock(g_mutex);
    c_group->admin = client;
    client->g_id = c_group->id;
}

void handle_free_client(char *msg, int cc, client_p client) {
    cc = normalize(msg, cc);
    printf("%d says:|%s|\n", client->sock, msg);
    if (strcmp(msg, "GETOPENGROUPS") == 0) {
        char *out_msg = NULL;
        cc = open_groups(&out_msg);
        cc = (int) write(client->sock, out_msg, (size_t) cc);
        if (cc <= 0) {
            destroy_client(client);
        }
        return;
    }
    char *tokens[MAX_TOKEN_CNT];
    cc = parse_args(msg, tokens);
    if (strcmp(tokens[0], "JOIN") == 0) {
        if (cc != 3) {
            write(client->sock, "BAD|Wrong format\r\n", 18);
            return;
        }
        remove_member(0, client);
        int g_id = find_group(tokens[1]);
        if (g_id < 0) {
            cc = (int) write(client->sock, "BAD|No such group\r\n", 19);
            if (cc <= 0) {
                destroy_client(client);
            }
            return;
        }
        client->n_len = (int) strlen(tokens[2]);
        client->name = calloc(strlen(tokens[2]) + 1, sizeof(char));
        strcpy(client->name, tokens[2]);
        int status = add_member(g_id, client);
        if (status == 0) {
            cc = (int) write(client->sock, "OK\r\n", 4);
            if (cc <= 0) {
                destroy_client(client);
            }
            return;
        }
        if (status == 2) {
            cc = (int) write(client->sock, "BAD|Group is full\r\n", 19);
            if (cc <= 0) {
                destroy_client(client);
            }
            return;
        }
        if (status == 3) {
            cc = (int) write(client->sock, "BAD|Already there\r\n", 19);
            if (cc <= 0) {
                destroy_client(client);
            }
            return;
        }
        return;
    }
}

void *new_client(void *args) {
    int sock = (int) args;
    printf("new thread here, %d has come!\n", sock);
    fflush(stdout);
    client_p client = create_client(sock);
    if (client == NULL) {
        printf("Couldn't create a client_t for %d\n", sock);
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
    pthread_mutex_lock(g_mutex);
    int status = add_member(0, client);
    pthread_mutex_unlock(g_mutex);
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

void *hub(void *args) {
    group_p group = all_groups;
    fd_set set;
    struct timeval timeout;
    int mx, cc;
    char *buffer = calloc(MAX_BUFFER_SIZE, sizeof(char));
    printf("Hub started!\n");
    fflush(stdout);
    for (;;) {
        pthread_mutex_lock(g_mutex);
        FD_ZERO(&set);
        timeout.tv_sec = 3;
        timeout.tv_usec = 0;
        mx = 0;
        for (int i = 0; i < group->c_size; i++) {
            client_p c_client = group->members[i];
            FD_SET(c_client->sock, &set);
            if (c_client->sock > mx)
                mx = c_client->sock;
        }
        pthread_mutex_unlock(g_mutex);

        printf("round again. %d\n", mx);
        select(mx + 1, &set, NULL, NULL, &timeout);

        pthread_mutex_lock(g_mutex);
        for (int i = 0; i < group->c_size; i++) {
            client_p c_client = group->members[i];
            if (FD_ISSET(c_client->sock, &set)) {
                cc = (int) read(c_client->sock, buffer, MAX_BUFFER_SIZE);
                if (cc <= 0) {
                    destroy_client(c_client);
                    continue;
                }
                handle_free_client(buffer, cc, c_client);
            }
        }
        fflush(stdout);
        pthread_mutex_unlock(g_mutex);
    }
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
        ssock = accept(msock, (struct sockaddr *) &fsin, (socklen_t *) & alen);
        if (ssock < 0) {
            fprintf(stderr, "accept: %s\n", strerror(errno));
            exit(-1);
        }

        printf("A client_t has arrived for echoes.\n");
        fflush(stdout);
        pthread_t temp_pthread;
        pthread_create(&temp_pthread, NULL, new_client, (void *) ssock);
    }
}
