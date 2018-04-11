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
#define SEPARATOR "|"
#define QLEN 5

typedef struct question question;
typedef struct client client;
typedef struct quiz_group quiz_group;

struct question {
    char * str;
    int q_len;
    char * ans;
    int a_len;
};

struct client {
    int id;
    int ssock;
    char * name;
    int n_len;
    int point;
    quiz_group * group;
};

struct quiz_group {
    int id;
    char * name;
    int n_len;
    int current_size;
    int dedicated_size;
    client * admin;
    client * members; // array for the members of the group
};

// 0 is for the "hub".
// NULL, if there is no such
quiz_group all_groups[33];
// Mutex for the each group, so object can be safely used
pthread_mutex_t groups_mutex[33];
pthread_t all_threads[33];
int last_thread = 0;

// there will be at most 1020 clients
client all_clients[1024];
int last_client;

// In order to normalize string = remove trailing \r\n
int normalize(char * str, int len);
// Thread-safe parsing of the arguments by "|"
int parse_args(char * str, char ** args);
// Sort members by their points
void sort_members(quiz_group * q_group);

// reads message from client terminated with CRLF
// returns status of read
// 0 - ok
// 1 - error, occured
int read_msg_cr(client * cl, char * str);
// reads message from client with given size in the message
// returns status of read
int read_msg_size(client * cl, char * str);

// Safely (mutex) adds and removes member to the group
// returns status of the operation:
// 0 - ok
// add: 1 - no such group, 2 - full group, 3 - already there
// remove: 1 - no such group, 2 - no such client
int add_member(char * group_name, client * cl);
int remove_member(char * group_name, client * cl);

// Creates client, checks if there is already
client * create_member(int sock);
// Close connection, remove from groups, and all lists
void close_connection(client * cl);

// Hub function
void * hub(void * args);
// quiz-group function
void * group_thread(void * args);
