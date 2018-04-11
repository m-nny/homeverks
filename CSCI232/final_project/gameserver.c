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
#define     QUESTIONSIZE    2048
#define     THREADS         100
#define     SEPARATOR       "|"
#define     ARGCNT          5
#define     WELCOMESTR      "QS|ADMIN\r\n"
#define     JOINSTR         "QS|JOIN\r\n"
#define     FULLSTR         "QS|FULL\r\n"
#define     WAITSTR         "WAIT\r\n"
#define     NOANSSTR        "ANS|NOANS"
#define     sock_exit       {close(ssock); pthread_exit(NULL);}
#define     sock_assert(x)  if (!(x)) {printf("Some error occurred\n"); sock_exit }
#define     sock_assert2(x)  if (!(x)) {\
                                printf("Some error occurred\n"); \
                                pthread_mutex_lock(&mutex);\
                                current_group_size--;\
                                pthread_mutex_unlock(&mutex);\
                                sock_exit}

int passivesock(char *service, char *protocol, int qlen, int *rport);

int last_client_id = 0;
int group_size = 0;
int current_group_size = 0;
int answered_size = 0;
int ready_to_start = 0;
char *client_names[THREADS];
pthread_mutex_t mutex;
//pthread_mutex_t cond_mutex;
pthread_cond_t cond;
FILE *input_file;
char *question, *answer;
char *scoreboard;
int q_len, a_len, winner = -1;
int admin_is_here = 0;
int points[THREADS];

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

void read_question() {
    question = calloc(QUESTIONSIZE, sizeof(char));
    char * buf = calloc(QUESTIONSIZE, sizeof(char));
    answer = calloc(QUESTIONSIZE, sizeof(char));
    q_len = 0;
    do {
        buf[q_len++] = (char) fgetc(input_file);
        if (buf[q_len - 1] == EOF) {
            q_len = 2;
            break;
        }
    } while (!(q_len >= 2 && buf[q_len - 2] == '\n' && buf[q_len - 1] == '\n'));
    buf[q_len - 2] = 0;
    if (q_len == 2) {
        printf("Empty question\n");
        return;
    }
    sprintf(question, "QUES|%d|%s\r\n", q_len, buf);
    q_len = (int) strlen(question);

    a_len = 0;
    do {
        buf[a_len++] = (char) fgetc(input_file);
    } while (!(a_len >= 2 && buf[a_len - 2] == '\n' && buf[a_len - 1] == '\n'));
    buf[a_len - 2] = 0;
    sprintf(answer, "ANS|%s", buf);

    answered_size = 0;
    winner = -1;
    printf("|%s|%s|\n%d %d\n", question, answer, q_len, a_len);
}

void make_scoreboard() {
    scoreboard = calloc(BUFSIZE, sizeof(char));
    int offset = 0;
    offset = sprintf(scoreboard, "RESULT");
    for (int i = 0; i < last_client_id; i++) {
        offset += sprintf(scoreboard + offset, "|%s|%d", client_names[i], points[i]);
    }
    sprintf(scoreboard + offset, "\r\n");
}

void cleanup() {
    free(question);
    free(answer);
    free(scoreboard);
    question = NULL;
    answer = NULL;
    scoreboard = NULL;
    q_len = 0;
    a_len = 0;
    //winner = -1;
}

void *handle_client(void *arg) {
    char buf[BUFSIZE];
    char *msg[ARGCNT];
    int cc;
    int ssock = (int) arg;
    int msg_c;
    int status = -1;
    int client_id = -1;

    pthread_mutex_lock(&mutex);
    if (admin_is_here == 0) {
        admin_is_here = 1;
        status = 1;
    } else if (current_group_size >= group_size) {
        status = 2;
    }
    pthread_mutex_unlock(&mutex);

    if (status == 2) {
        sock_assert ((cc = (int) write(ssock, FULLSTR, strlen(FULLSTR))) > 0);
        close(ssock);
        pthread_exit(NULL);
    }

    if (status == 1) {
        if ((cc = (int) write(ssock, WELCOMESTR, strlen(WELCOMESTR))) <= 0) {
            admin_is_here = 0;
            sock_exit;
        }
        if ((cc = (int) read(ssock, buf, BUFSIZE)) <= 0) {
            admin_is_here = 0;
            sock_exit;
        }
        cc = normalize(buf, cc);
        printf("The client says: |[%s]|\n", buf);

        msg_c = parseArguments(buf, msg);
        if (msg_c == 3 && strcmp(msg[0], "GROUP") != 0) {
            admin_is_here = 0;
            sock_exit;
        }
        printf("[%s(%d), %s(%d), %s(%d)]\n", msg[0], (int) strlen(msg[0]),
               msg[1], (int) strlen(msg[1]), msg[2], (int) strlen(msg[2]));
        if ((cc = (int) write(ssock, WAITSTR, strlen(WAITSTR))) <= 0) {
            admin_is_here = 0;
            sock_exit;
        }

        pthread_mutex_lock(&mutex);
        client_id = last_client_id++;
        client_names[client_id] = calloc(strlen(msg[1]), sizeof(char));
        strcpy(client_names[client_id], msg[1]);
        group_size = atoi(msg[2]);
        current_group_size++;

        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }
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
        client_id = last_client_id++;
        client_names[client_id] = calloc(strlen(msg[1]), sizeof(char));
        strcpy(client_names[client_id], msg[1]);
        current_group_size++;

        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }
    printf("%s has joined.\n", client_names[client_id]);
    printf("%d out of %d\n", current_group_size, group_size);
    fflush(stdout);

    pthread_mutex_lock(&mutex);
    while (current_group_size < group_size) {
        pthread_cond_wait(&cond, &mutex);
    }
    pthread_mutex_unlock(&mutex);

    for (int my_q_id = 0; ; my_q_id++) {
        pthread_mutex_lock(&mutex);
        ready_to_start++;
        pthread_cond_broadcast(&cond);

        if (ready_to_start == 1) {
            read_question();
        }

        if (ready_to_start == current_group_size) {
            answered_size = 0;
        }
        while (ready_to_start < current_group_size) {
            pthread_cond_wait(&cond, &mutex);
        }
        pthread_mutex_unlock(&mutex);

        if (q_len == 2) {
            // END OF QUIZ
            break;
        }


        sock_assert2 ((cc = (int) write(ssock, question, (size_t) q_len)) > 0);

        sock_assert2 ((cc = (int) read(ssock, buf, BUFSIZE)) > 0);
        cc = normalize(buf, cc);
        printf("The client says: |[%s]|\n", buf);
        fflush(stdout);
        if (strcmp(buf, NOANSSTR) != 0) {
            status = strcmp(buf, answer) == 0;
            if (status > 0) {
                pthread_mutex_lock(&mutex);
                if (winner == -1) {
                    winner = client_id;
                    status = 2;
                }
                pthread_mutex_unlock(&mutex);
            }

            if (status == 2) {
                points[client_id] += 2;
            }
            if (status == 1) {
                points[client_id] += 1;
            }
            if (status == 0) {
                points[client_id] -= 1;
            }
        }
        pthread_mutex_lock(&mutex);
        answered_size++;
        pthread_cond_broadcast(&cond);

        printf("%d out of %d\n", answered_size, current_group_size);
        printf("%s answered for %d points\n", client_names[client_id], points[client_id]);

        if (answered_size == current_group_size) {
            printf("All of client's answered\n");
            ready_to_start = 0;
            cleanup();
        }

        while (answered_size < current_group_size) {
            pthread_cond_wait(&cond, &mutex);
        }
        pthread_mutex_unlock(&mutex);
        if (winner != -1) {
            sprintf(buf, "WIN|%s\r\n", client_names[winner]);
        } else {
            sprintf(buf, "WIN|NOWINNER\r\n");
        }
        sock_assert2(write(ssock, buf, strlen(buf)) > 0);
    }

    pthread_mutex_lock(&mutex);
    if (scoreboard == NULL) {
        make_scoreboard();
    }
    pthread_mutex_unlock(&mutex);

    sock_assert2 ((cc = (int) write(ssock, scoreboard, strlen(scoreboard))) > 0);
    pthread_mutex_lock(&mutex);
    current_group_size--;
    if (current_group_size == 0) {
        cleanup();
        rewind(input_file);
        admin_is_here = 0;
        ready_to_start = 0;
        last_client_id = 0;
    }
    pthread_mutex_unlock(&mutex);
    sock_exit;
}

int main(int argc, char *argv[]) {
    char *service;
    struct sockaddr_in fsin;
    int alen;
    int msock;
    int rport = 0;
    pthread_t threads[THREADS];
    int last_thread = 0;
    char *filename;
    switch (argc) {
        case 2:
            // one arg? let the OS choose a port and tell the user
            filename = argv[1];
            rport = 1;
            break;
        case 3:
            // User provides a port? then use it
            filename = argv[1];
            service = argv[2];
            break;
        default:
            fprintf(stderr, "usage: server quiz_file [port]\n");
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
    input_file = fopen(filename, "r");
    if (input_file == NULL) {
        fprintf(stderr, "There was error opening quiz file\n");
        return 0;
    }

    pthread_mutex_init(&mutex, NULL);
//    pthread_mutex_init(&cond_mutex, NULL);
    pthread_cond_init(&cond, NULL);
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
