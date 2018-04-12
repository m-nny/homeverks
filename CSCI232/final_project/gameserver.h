//
// Created by Alibek Manabayev on 4/12/18.
//

#ifndef PROJECT_GAMESERVER_H
#define PROJECT_GAMESERVER_H

#endif //PROJECT_GAMESERVER_H

#define MAX_GROUP_NUM     32
#define MAX_CLIENT_NUM    1020
#define MAX_QUEST_LEN     2048
#define MAX_QUEST_NUM     128
#define MAX_BUFFER_SIZE   2048
#define SEPARATOR "|"
#define QLEN 5

struct question {
    char * str;
    int q_len;
    char * ans;
    int a_len;
};

struct client {
    int id;
    int sock;
    char * name;
    int n_len;
    int point;
    struct quiz_group * group;
};

struct q_group {
    char * topic;
    int t_len;
    int id;
    char * name;
    int n_len;
    int c_size;
    int d_size;
    struct client * admin;
    struct client ** members; // array for the members of the group
};

typedef struct question question;
typedef struct client client;
typedef struct q_group q_group;

// 0 is for the "hub".
// NULL, if there is no such
q_group all_groups[33];
int l_group = 0;

// Mutex for the each group, so object can be safely used
pthread_mutex_t g_mutex[33];
pthread_t all_threads[33];
int l_thread = 0;

// there will be at most 1020 clients
client all_clients[1024];
int l_client = 0;

// In order to normalize string = remove trailing \r\n
int normalize(char * str, int len);
// Thread-safe parsing of the arguments by "|"
int parse_args(char * str, char ** args);
// Sort members by their points
void sort_members(q_group * q_group);

// reads message from client terminated with CRLF
// returns status of read
// 0 - ok
// 1 - error, occured
int read_msg_cr(client * cl, char * str);
// reads message from client with given size in the message
// returns status of read
int read_msg_size(client * cl, char * str);

int find_group(char * group_name);
int open_groups(char **str);

// Safely (mutex) adds and removes member to the group
// returns status of the operation:
// 0 - ok
// add: 1 - no such group, 2 - full group, 3 - already there
// remove: 1 - no such group, 2 - no such client
int add_member(int g_id, client * cl);
int remove_member(int g_id, client * cl);

// Only creates client object
client * create_member(int sock);
// Close connection, remove from groups, and all lists
void close_connection(client * cl);

// Hub function
void * hub(void * args);
// quiz-group function
void * group_thread(void * args);
