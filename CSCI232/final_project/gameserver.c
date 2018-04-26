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
    client->gid = -1;
    free(client->name);
    client->name = NULL;
    client->n_len = 0;
    client->point = 0;
    client->sock = sock;
    return client;
}

int destroy_client(client_p client) {
    pthread_mutex_lock(g_mutex + client->gid);
    remove_member(client->gid, client);
    pthread_mutex_unlock(g_mutex + client->gid);
    close(client->sock);
    free(client->name);

    client->name = NULL;
//    free(client);
    return 0;
}

int destroy_question(question_p quest) {
    free(quest->ans);
    free(quest->str);
    return 1;
}

group_p create_group(int id, int dedicated_size) {
    ///TO CHANGE
    id = l_group;
    group_p group = all_groups + (l_group++);
    group->c_size = 0;
    group->d_size = dedicated_size;
    free(group->members);
    group->members = calloc((size_t) dedicated_size, sizeof(client_p));
    group->id = id;
    group->admin = -1;
    free(group->name);
    group->name = NULL;
    group->n_len = -1;
    free(group->topic);
    group->topic = NULL;
    group->t_len = -1;
    group->question = calloc(MAX_QUEST_NUM, sizeof(question_p));
    group->q_num = 0;
    group->started = 0;
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

int add_member(int gid, client_p client) {
    if (gid < 0 || gid > MAX_GROUP_NUM)
        return 1;
    group_p group = all_groups + gid;
    if (group->d_size <= 0) {
        return 1;
    }
    if (group->c_size == group->d_size) {
        return 2;
    }
    if (group->started) {
        return 4;
    }
    for (int i = 0; i < group->d_size; i++) {
        if (group->members[i] == NULL)
            continue;
        if (group->members[i]->id == client->id) {
            return 3;
        }
    }
    group->members[group->c_size] = client;
    group->c_size++;
    client->gid = gid;
    printf("Client %d joined %d group\n", client->id, gid);
    fflush(stdout);
    return 0;
}

int remove_member(int gid, client_p client) {
    if (gid < 0 || gid > MAX_GROUP_NUM)
        return 1;
    group_p group = all_groups + gid;
    if (group->d_size <= 0) {
        return 1;
    }
    for (int i = 0; i < group->d_size; i++) {
        if (group->members[i] == NULL)
            continue;
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
//        printf("[%d] MUTEX LOCK (OPENGROUPS)\n", i);
//        fflush(stdout);
        if (group->d_size > 0 && !group->started)
            offset += sprintf(str + offset, "|%s|%s|%d|%d", group->topic, group->name,
                              group->d_size, group->c_size);
//        printf("[%d] MUTEX UNLOCK (OPENGROUPS)\n", i);
//        fflush(stdout);
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

question_p parse_question(FILE *input_file) {
    question_p quest = malloc(sizeof(question_t));
    quest->str = calloc(MAX_QUEST_LEN, sizeof(char));
    char *buf = calloc(MAX_QUEST_LEN, sizeof(char));
    quest->ans = calloc(MAX_QUEST_LEN, sizeof(char));
    int q_len = 0;
    do {
        buf[q_len++] = (char) fgetc(input_file);
        if (buf[q_len - 1] == EOF) {
            free(quest);
            return NULL;
        }
    } while (!(q_len >= 2 && buf[q_len - 2] == '\n' && buf[q_len - 1] == '\n'));
    q_len -= 2;
    buf[q_len] = 0;
    if (q_len == 0) {
        printf("Empty question\n");
        free(quest);
        return NULL;
    }
    sprintf(quest->str, "%s\r\n", buf);
    q_len = (int) strlen(quest->str);

    int a_len = 0;
    do {
        buf[a_len++] = (char) fgetc(input_file);
    } while (!(a_len >= 2 && buf[a_len - 2] == '\n' && buf[a_len - 1] == '\n'));
    a_len -= 2;
    buf[a_len] = 0;
    sprintf(quest->ans, "%s\r\n", buf);
    quest->q_len = q_len;
    quest->a_len = a_len;
    return quest;
}

void parse_quiz(char *string, group_p group) {
    char *buffer = calloc(20, sizeof(char));
    sprintf(buffer, "%s/tmp_file_%d.txt", TEMPORARY_FOLDER, group->id);
    FILE *tmp_file = fopen(buffer, "w+");
    if (tmp_file == NULL) {
        printf("Some shit occurred while opening file %s", buffer);
        exit(1);
    }
    fputs(string, tmp_file);
    fseek(tmp_file, 0, SEEK_SET);
    int q_num = 0;
    question_p question = NULL;
    while ((question = parse_question(tmp_file)) != NULL) {
        group->question[q_num] = question;
        printf("|%s|(%d)\n", group->question[q_num]->str, group->question[q_num]->q_len);
        printf("|%s|(%d)\n", group->question[q_num]->ans, group->question[q_num]->a_len);
        q_num++;
    }
    group->q_num = q_num;
    fclose(tmp_file);
}

int read_quiz(client_p client, group_p group) {
    int b_len = MAX_QUEST_NUM * MAX_QUEST_LEN + 20;
    char *buffer = calloc((size_t) b_len + 1, sizeof(char));
    int offset;
    int need_size = b_len;
    int cc;
    char *header = calloc(21, sizeof(char));
    cc = (int) read(client->sock, header, 20);
    if (cc <= 0) {
        return -1;
    }
    header[cc] = 0;
    int pp = sscanf(header, "QUIZ|%d|", &need_size);
    if (pp < 1) {
        cc = (int) write(client->sock, "BAD|Wrong format\r\n", 18);
        if (cc <= 0) {
            destroy_client(client);
            return -1;
        }
        return -2;
    }
    offset = -1;
    for (int i = 0; i < cc; i++) {
        if (header[i] == '|') {
            if (offset == -1) {
                offset = -2;
            } else {
                offset = i + 1;
                break;
            }
        }
    }
    if (offset < 0) {
        cc = (int) write(client->sock, "BAD|Wrong format\r\n", 18);
        if (cc <= 0) {
            destroy_client(client);
            return -1;
        }
        return -2;
    }
    strcpy(buffer, header + offset);
    need_size -= cc - offset;
    printf("it is QUIZ|%d|%s|total:%d\n", need_size, buffer, cc);
    fflush(stdout);
    do {
        cc = (int) read(client->sock, buffer + offset, (size_t) need_size);
//        printf("Client says|%s|%d| need|%d|\n", buffer + offset, cc, need_size);
        if (cc <= 0) {
            write(client->sock, "BAD|nz\r\n", 8);
            return -1;
        }
        offset += cc;
        need_size -= cc;
    } while (need_size > 0);
    write(client->sock, "OK\r\n", 4);
    printf("SENDING OK\n");
    fflush(stdout);
    parse_quiz(buffer, group);
    return 1;
}

void init() {
    group_p hub_group = create_group(0, MAX_CLIENT_NUM);
    pthread_create(all_threads + (hub_group->id), NULL, hub, NULL);

//    /// TESTING ONLY
//    group_p test_group = create_group(1, 2);
//    test_group->topic = calloc(12, sizeof(char));
//    strcpy(test_group->topic, "some_topic");
//    test_group->name = calloc(12, sizeof(char));
//    strcpy(test_group->name, "some_name");
//    /// TESTING ONLY
}

void handle_join(char **tokens, int cc, client_p client) {
    if (cc != 3) {
        write(client->sock, "BAD|Wrong format\r\n", 18);
        return;
    }
    remove_member(0, client);
    int gid = find_group(tokens[1]);
    if (gid < 0) {
        cc = (int) write(client->sock, "BAD|No such group\r\n", 19);
        if (cc <= 0) {
            destroy_client(client);
        }
        return;
    }
    set_string(tokens[2], &(client->name), &(client->n_len));
    pthread_mutex_lock(g_mutex + gid);
//    printf("[%d] MUTEX LOCK (handle join)\n", gid);
//    fflush(stdout);
    int status = add_member(gid, client);
//    printf("[%d] MUTEX UNLOCK (handle join)\n", gid);
//    fflush(stdout);
    pthread_mutex_unlock(g_mutex + gid);
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

void handle_create_group(char **tokens, int cc, client_p client) {
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
    group_p group = create_group(-1, g_size);
    set_string(tokens[1], &(group->topic), &(group->t_len));
    set_string(tokens[2], &(group->name), &(group->n_len));
    group->d_size = g_size;

    cc = (int) write(client->sock, "SENDQUIZ\r\n", 10);
    if (cc <= 0) {
        destroy_client(client);
    }
    cc = read_quiz(client, group);
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
//    printf("[%d] MUTEX LOCK (handle group)\n", 0);
//    fflush(stdout);
    remove_member(0, client);
//    printf("[%d] MUTEX UNLOCK (handle join)\n", 0);
//    fflush(stdout);
    pthread_mutex_unlock(g_mutex);
    group->admin = client->id;
    client->gid = group->id;
    pthread_create(all_threads + (group->id), NULL, group_thread, (void *) group->id);
}

void handle_free_client(char *msg, int cc, client_p client) {
    cc = normalize(msg, cc);
    printf("%d says:|%s|\n", client->sock, msg);
    if (strcmp(msg, "GETOPENGROUPS") == 0) {
        fflush(stdout);
        char *out_msg = NULL;
        cc = open_groups(&out_msg);
        printf("It was open groups:[%s]\n", out_msg);
        cc = (int) write(client->sock, out_msg, (size_t) cc);
        if (cc <= 0) {
            destroy_client(client);
        }
        return;
    }
    char *tokens[MAX_TOKEN_CNT];
    cc = parse_args(msg, tokens);
    if (strcmp(tokens[0], "JOIN") == 0) {
        printf("It was open JOIN:[]\n");
        handle_join(tokens, cc, client);
    }
    if (strcmp(tokens[0], "GROUP") == 0) {
        printf("It was open GROUP:[]\n");
        handle_create_group(tokens, cc, client);
    }
}

void handle_leave(client_p client) {
    int gid = client->gid;
    int status, cc = 0;
    pthread_mutex_lock(g_mutex + gid);
    status = remove_member(gid, client);
    pthread_mutex_unlock(g_mutex + gid);
    gid = 0;
    if (!status) {
        pthread_mutex_lock(g_mutex + gid);
        status = add_member(gid, client);
        pthread_mutex_unlock(g_mutex + gid);
    }
    if (!status) {
        cc = (int) write(client->sock, "OK\r\n", 4);
    } else {
        cc = (int) write(client->sock, "BAD\r\n", 4);
    }
    if (cc <= 0) {
        destroy_client(client);
    }
}

void *group_thread(void *args) {
    int gid = (int) args;
    group_p group = all_groups + gid;
    int last = group->c_size;
    fd_set set;
    struct timeval timeout;
    int mx, cc;
    char *buffer = calloc(MAX_BUFFER_SIZE, sizeof(char));
    char *tokens[MAX_TOKEN_CNT];
    printf("[%d] New group starting:\ntopic|%s|name|%s|d_size|%d|q_num|%d\n", gid, group->topic, group->name,
           group->d_size, group->q_num);
    printf("[%d] First one %d out of %d\n", gid, last, group->d_size);
    fflush(stdout);

    pthread_mutex_lock(g_mutex);
//    printf("[%d] MUTEX LOCK\n", group->id);
//    fflush(stdout);
    while (group->c_size < group->d_size) {
        FD_ZERO(&set);
        timeout.tv_sec = 3;
        timeout.tv_usec = 0;
        mx = 0;
        for (int i = 0; i < group->d_size; i++) {
            client_p client = group->members[i];
            if (client == NULL)
                continue;
            FD_SET(client->sock, &set);
            if (client->sock > mx)
                mx = client->sock;
        }
//        printf("[%d] MUTEX UNLOCK\n", group->id);
//        fflush(stdout);
        pthread_mutex_unlock(g_mutex);

        printf("[%d] round again. %d\n", gid, mx);
        select(mx + 1, &set, NULL, NULL, &timeout);

        for (int i = 0; i < group->d_size; i++) {
            client_p client = group->members[i];
            if (client == NULL)
                continue;
            if (FD_ISSET(client->sock, &set)) {
                cc = (int) read(client->sock, buffer, MAX_BUFFER_SIZE);
                if (cc <= 0) {
                    destroy_client(client);
                    continue;
                }
                cc = normalize(buffer, cc);
                printf("[%d] Client (%s) says |%s|\n", gid, client->name, buffer);
                if (!strcmp(buffer, "LEAVE")) {
                    handle_leave(client);
                }
            }
        }
        pthread_mutex_lock(g_mutex);
    }

//    printf("[%d] MUTEX UNLOCK (group thread)\n", gid);
//    fflush(stdout);
    printf("[%d] All client of group %d are here\n", gid, gid);
    fflush(stdout);
    for (int q = 0; q < group->q_num; q++) {
        int answered = 0;
        fd_set tmp_set;
        question_p question = group->question[q];
        mx = 0;
        printf("[%d] Sending question #%d\n", gid, q);
        fflush(stdout);
        FD_ZERO(&set);
        for (int i = 0; i < group->d_size; i++) {
            client_p client = group->members[i];
            if (!client || client->id == group->admin)
                continue;
            FD_SET(client->sock, &set);
            cc = (int) write(client->sock, question->str, (size_t) question->q_len);
            if (cc <= 0) {
                destroy_client(client);
                continue;
            }
            if (client->sock > mx)
                mx = client->sock;
        }
        printf("[%d] Done sending questions\n", gid);
        fflush(stdout);
        char *winner = NULL;
        while (answered < group->c_size) {
            memcpy((char *) &tmp_set, (char *) &set, sizeof(tmp_set));
            select(mx + 1, &tmp_set, NULL, NULL, NULL);
            for (int i = 0; i < group->d_size; i++) {
                client_p client = group->members[i];
                if (client == NULL)
                    continue;
                if (FD_ISSET(client->sock, &tmp_set)) {
                    cc = (int) read(client->sock, buffer, MAX_BUFFER_SIZE);
                    if (cc <= 0) {
                        destroy_client(client);
                        continue;
                    }
                    cc = normalize(buffer, cc);
                    printf("[%d] Quiz: Client (%s) says |%s|\n", gid, client->name, buffer);
                    ///TODO: check tokens
                    cc = parse_args(buffer, tokens);
                    if (cc != 2) {
                        cc = (int) write(client->sock, "BAD|Wrong format\r\n", 18);
                        if (cc <= 0) {
                            destroy_client(client);
                        }
                        continue;
                    }
                    int cur_pts = 0;
                    if (strcmp(tokens[1], "NOANS") == 0) {
                        cur_pts = 0;
                    } else if (strcmp(tokens[1], question->ans) == 0) {
                        if (winner == NULL) {
                            winner = client->name;
                            cur_pts = 2;
                        } else {
                            cur_pts = 1;
                        }
                    } else {
                        cur_pts = -1;
                    }
                    client->point += cur_pts;
                    FD_CLR(client->sock, &set);
                    answered++;
                    printf("[%d] Quiz: (%s) answer for %d points\n", gid, client->name, cur_pts);
                    fflush(stdout);
                }
            }
        }
        if (winner == NULL) {
            winner = calloc(4, sizeof(char));
            sprintf(winner, "\r\n");
        }
        printf("[%d] All clients answered. Winner is |%s|\n", gid, winner);
        sprintf(buffer, "WIN|%s\r\n", winner);
        for (int i = 0; i < group->d_size; i++) {
            client_p client = group->members[i];
            if (!client)
                continue;
            cc = (int) write(client->sock, buffer, strlen(buffer));
            if (cc <= 0) {
                destroy_client(client);
                continue;
            }
        }

        if (group->c_size == 0) {
            printf("[%d] No clients left.\n", gid);
            fflush(stdout);
            break;
        }
    }
    printf("Quiz ended. Yeap");
    fflush(stdout);
    for (int i = 0; i < group->d_size; i++) {
        client_p client = group->members[i];
        if (client == NULL)
            continue;
        remove_member(gid, client);
    }
    destroy_group(gid);
    pthread_exit(NULL);
}

void *new_client(void *args) {
    int sock = (int) args;
    printf("new thread here, %ds has come!\n", sock);
    fflush(stdout);
    client_p client = create_client(sock);
    if (client == NULL) {
        printf("Couldn't create a client for %ds\n", sock);
        pthread_exit(NULL);
    }
    char *msg_to_send = NULL;
    int m_len;
    m_len = open_groups(&msg_to_send);
    int cc = (int) write(sock, msg_to_send, (size_t) m_len);
    if (cc <= 0) {
        printf("Client %d has gone\n", client->id);
        pthread_exit(NULL);
    }
    pthread_mutex_lock(g_mutex);
//    printf("[%d] MUTEX LOCK (new client)\n", 0);
//    fflush(stdout);
    int status = add_member(0, client);
//    printf("[%d] MUTEX UNLOCK (new client)\n", 0);
//    fflush(stdout);
    pthread_mutex_unlock(g_mutex);
    if (status == 0) {
        printf("Client %d joined hub\n", client->id);
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
        for (int i = 0; i < group->d_size; i++) {
            client_p client = group->members[i];
            if (client == NULL)
                continue;
            FD_SET(client->sock, &set);
            if (client->sock > mx)
                mx = client->sock;
        }
        pthread_mutex_unlock(g_mutex);

        printf("round again. %d\n", mx);
        select(mx + 1, &set, NULL, NULL, &timeout);

        for (int i = 0; i < group->d_size; i++) {
            client_p client = group->members[i];
            if (client == NULL)
                continue;
            if (FD_ISSET(client->sock, &set)) {
                cc = (int) read(client->sock, buffer, MAX_BUFFER_SIZE);
                if (cc <= 0) {
                    destroy_client(client);
                    continue;
                }
                handle_free_client(buffer, cc, client);
            }
        }
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
