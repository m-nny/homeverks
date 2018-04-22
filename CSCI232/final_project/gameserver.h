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
#define MAX_TOKEN_CNT     15
#define MAX_BUFFER_SIZE   2048
#define SEPARATOR "|"
#define QLEN 5

struct question_struct {
    char *str;
    int q_len;
    char *ans;
    int a_len;
};

struct client_struct {
    int id;
    int sock;
    char *name;
    int n_len;
    int point;
    int g_id;
};

struct group_struct {
    char *topic;
    int t_len;
    int id;
    char *name;
    int n_len;
    int c_size;
    int d_size;
    struct client_struct *admin;
    struct client_struct **members; // array for the members of the group
};

typedef struct question_struct question_t;
typedef struct client_struct client_t;
typedef struct group_struct group_t;

typedef struct question_struct *question_p;
typedef struct client_struct *client_p;
typedef struct group_struct *group_p;

// 0 is for the "hub".
// NULL, if there is no such
group_t all_groups[33];
int l_group = 0;

// Mutex for the each group, so object can be safely used
pthread_mutex_t g_mutex[33];
pthread_t all_threads[33];
int l_thread = 0;

// there will be at most 1020 clients
client_t all_clients[1024];
int l_client = 0;

// In order to normalize string = remove trailing \r\n
int normalize(char *str, int len);

// Thread-safe parsing of the arguments by "|"
int parse_args(char *str, char **args);

// Sort members by their points
void sort_members(group_p q_group);

// reads message from client terminated with CRLF
// returns status of read
// 0 - ok
// 1 - error, occured
int read_msg_cr(client_p cl, char *str);

// reads message from client with given size in the message
// returns status of read
int read_msg_size(client_p cl, char *str);

int find_group(char *group_name);

int open_groups(char **str);

// adds and removes member to the group. Appropriate mutex should be locked before it
// returns status of the operation:
// 0 - ok
// add: 1 - no such group, 2 - full group, 3 - already there
// remove: 1 - no such group, 2 - no such client_t
int add_member(int g_id, client_p client);

int remove_member(int g_id, client_p client);

// Only creates client_t object
client_p create_client(int sock);
int destroy_client(client_p client);

group_p create_group(int id, int dedicated_size);
int destroy_group(int id);

// Close connection, remove from groups, and all lists
void close_connection(client_p cl);

// Hub function
void *hub(void *args);

// quiz-group function
void *group_thread(void *args);
