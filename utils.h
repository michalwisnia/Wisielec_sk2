#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>

struct Hang 
{
    char *word;
    struct Hang * next;
};

typedef struct auth_user auth_user;

struct auth_user {
    char * nickname;
    struct sockaddr_in address;
    int sockkfd;
    bool logged_on;
    size_t won;
    size_t played;
    size_t first_game_score;
    int score;
    struct auth_user * next;
};

typedef struct Leaderboard Leaderboard;

struct Leaderboard {
    uint8_t id;
    auth_user * first;
    auth_user * last;
    size_t players;
    struct Leaderboard * next;
};


void add_user( auth_user *first,auth_user *last,auth_user *user);

//void remove_user(auth_user *ptr,char *nickname);

struct auth_user* searchn(struct auth_user *ptr, char* nickname) ;

Leaderboard* create_new_room(uint8_t id);

Leaderboard* search_room(uint8_t id);

void add_to_room(uint8_t id, auth_user * user);


void read_file_word(void);

void add_words(char * word1);

char *printRandom(struct Hang *head);

void add_board (char * nickname,int won, int played );
