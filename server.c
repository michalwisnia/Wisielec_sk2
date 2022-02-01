#define _GNU_SOURCE
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include "utils.h"


#define DEFAULT_PORT 10000
#define BACKLOG 10
#define MAX 256

void* SendScore(int socket_id, uint8_t leaderboard_id);
int menu_input(int socket_id, char* nickname);
void Game_play(auth_user *user, uint8_t leaderboard_id);
int guesses_min(int words_length);
int hangman(int socket_id, char* nickname);
void *connection_handler(void *user);
void list_available_room(int socket_id);
void wait_for_player(int socket_id, size_t * player_number);


pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
struct auth_user *auth_user_first = NULL;
struct auth_user *auth_user_last = NULL;

struct Hang *Hang_first = NULL;
struct Hang *Hang_last = NULL;

pthread_mutex_t board_mutex = PTHREAD_MUTEX_INITIALIZER;
struct Leaderboard *Leaderboard_first =  NULL;
struct Leaderboard *Leaderboard_last = NULL;
size_t number_of_rooms = 0;

typedef enum {
    _,
    PLAY,
    RANKING,
    QUIT
} Menu;

char **users;

int won = 0;
int played = 0;

int sockfd;

int hangman(int socket_id, char* username){

    char *results = printRandom(Hang_first);

    char str[200];
    char* word_cat = results;
    char *ptr;

    strcpy (str, word_cat);
    strtok_r (str, ",", &ptr);

    char* word = str;
    char* category = ptr;

        //for debugging
    printf("Slowo - %s kategoria - %s \n", word, category);
    printf("Dlugosc slowa %zu dlugosc kategorii %zu \n", strlen(word), strlen(category));

    if(send(socket_id, category, MAX, 0) <= 0){
        return -1;
    }

    char* client_guessed_letters = calloc(MAX, sizeof(char));

    int num_guesses = guesses_min(strlen(word) + 10);

    char cword[MAX];

    for(int i = 0; i < strlen(word); i++){
        cword[i] = 95;
    }
    cword[strlen(word)] = '\0';

    char recv_letter;

    int GameOver = 0;
    int winner = 0;
    uint16_t converter;

    while(1){

        if(num_guesses <= 0){
            GameOver = 1;
        }

        if(strcmp(word, cword) == 0 ){
            winner = 1;
        }


        converter = htons(GameOver);
        if(send(socket_id, &converter, sizeof(uint16_t), 0) <= 0){
            return -1;
        }

        converter = htons(winner);
        if(send(socket_id, &converter, sizeof(uint16_t), 0) <= 0){
            return -1;
        }

        if(send(socket_id, client_guessed_letters, MAX, 0) <= 0){
            return -1;
        }

        converter = htons(num_guesses);
        if(send(socket_id, &converter, sizeof(uint16_t), 0) <= 0){
            return -1;
        }

        if(send(socket_id, cword, MAX, 0) <= 0){
            return -1;
        }

        if(winner){
            printf("\nGracz %s wygral gre!\n", username);
            return 1;
        }

        if(GameOver){
            printf("\nGracz %s przegral gre!\n", username);
            return -2;
        }

        if(recv(socket_id, &recv_letter, sizeof(char), 0) <= 0){
            return -1;
        }

        num_guesses--;

        if(strchr(client_guessed_letters, recv_letter) == NULL){
            strcat(client_guessed_letters, &recv_letter);
        }

        for(int i = 0; i < strlen(word); i++){
            if (word[i] == recv_letter){
                cword[i] = word[i];
            }
        }

    }
    return 0;
}

void Game_play(auth_user *user, uint8_t leaderboard_id) {
    int input;
    do { 
        switch ((input = menu_input(user->sockkfd
                        ,user->nickname))) {
            case PLAY:
                user->played += 1;

                int flag = hangman(user->sockkfd
                        ,user->nickname);
                if(flag == 1){
                    user->won += 1;
                    user->score += 1;
                } else if (flag == -2){
                    user->score += -2;
                }
                if (user->played == 1){
                    user->first_game_score = flag;
                }

                break;
            case RANKING:

                SendScore(user->sockkfd, leaderboard_id);

                break;
        }
    } while (input != QUIT); 
    user->logged_on = false;
}

int menu_input(int socket_id, char* nickname) {
    uint16_t converter;
    int input = 0;
    if (recv(socket_id, &converter, sizeof(uint16_t), 0) <= 0){
        return -1;
    }
    input = ntohs(converter);
    printf("Menu option : %d \n", input);

    return input;
}

int compare_user(const void *a, const void *b){
    return (((auth_user*)a)->score - ((auth_user*) b)->score);
}

void* SendScore(int socket_id, uint8_t id){
    Leaderboard *ptr = search_room(id);
    //sorting rankingu
    int number_of_players = 0;
    int i = 0;
    auth_user *user = ptr->first;
    while(user != NULL){
        ++number_of_players;
        user = user->next;
    }
    user = ptr->first;
    auth_user arr[number_of_players];
    while(user != NULL){
        asprintf(&arr[i].nickname, "%s",user->nickname);
        arr[i].won = user->won;
        arr[i].played = user->played;
        arr[i].score = user->score;
        arr[i].first_game_score = user->first_game_score;
        arr[i].logged_on = user->logged_on;
        user = user->next;
        ++i;
    }
    --i;
    qsort(arr, number_of_players, sizeof(auth_user),compare_user);
    char *buf = NULL;

    size_t offset = 0;
    user = ptr->first;
    printf("wysylana tablica\n");
    int rank = 1;
    for(; i >= 0; i--){
        char *newbuf;
        if(arr[i].played >= 20){
            arr[i].score -= arr[i].first_game_score;
        }
        asprintf(&newbuf, "\n%d: %s (Status: %s)\nWynik: %d (Wygrane %lu gry na %lu rozegranych)\n",
                rank, 
                arr[i].nickname,
                arr[i].logged_on ? "online" : "offline",
                arr[i].score,
                arr[i].won,
                arr[i].played);
        printf("wysylam to : %s",newbuf);
        
        // apending newbuf
        if(buf == NULL){
            buf = newbuf; 
        }
        else{
            asprintf(&buf, "%s%s", buf, newbuf);
            free(newbuf);
        }
        free(arr[i].nickname);
        ++rank;
    }

    offset = strlen(buf);
    printf("\nwielkosc bufera %lu\n", offset);

    int16_t stuff = htons(offset);
    if(send(socket_id, &stuff, sizeof(int16_t), 0) <= 0){
        perror("blad numerow pokoju");
    }
    send(socket_id, buf, offset, 0);
    free(buf);
    return NULL;
}

int guesses_min(int words_length){
    if (words_length <= 26){
        return words_length;
    } else {
        return 26;
    }
}

void list_available_room(int socket_id){
    Leaderboard *ptr = Leaderboard_first;
    // sending the total number of rooms 
    int16_t stuff = htons(number_of_rooms);
    if(send(socket_id, &stuff, sizeof(int16_t), 0) <= 0){
        perror("error sending room numbers");
    }
    printf("Liczba pokoi  : %lun", number_of_rooms);
    // room loop
    while(ptr != NULL){
        
        char *buf = NULL;
        
        size_t offset = 0;
        auth_user *user = ptr->first;
        while(user != NULL){
            char *newbuf;
            if(user->logged_on){
                asprintf(&newbuf, "%s online\n", user->nickname);
            } else
                asprintf(&newbuf, "%s offline\n", user->nickname);
            printf("wysylam to : %s",newbuf);
            // apending newbuf to the old buf
            if(buf == NULL){
                buf = newbuf;
            }
            else {
                asprintf(&buf, "%s%s", buf, newbuf);
                free(newbuf);
            }
            user = user->next;
        }

        offset = strlen(buf);
        int16_t stuff = htons(offset);
        if(send(socket_id, &stuff, sizeof(int16_t), 0) <= 0){
            perror("error in sending numbers of room");
        }
        send(socket_id, buf, offset, 0);
        free(buf);
        ptr = ptr->next;
    }
}


void *connection_handler(void *arg){
    auth_user *user = (auth_user*)arg;
    uint8_t room_id;

    recv(user->sockkfd, user->nickname, 20,0);
    printf("uzytkownik to %s\n", user->nickname);
    list_available_room(user->sockkfd);
    uint16_t converter;
    if(recv(user->sockkfd, &converter, sizeof(uint16_t), 0) <= 0)
        perror("error recieving room code");
    room_id = (uint8_t)htons(converter);
    printf("\n Nr pokoju od uzytkownika %s to: %d\n", user->nickname, room_id);
    Leaderboard * room = search_room(room_id);
    if(room == NULL){
        printf("pokoj nie istnieje, wiec tworzony jest nowy pokoj");
        room = create_new_room(room_id);
        number_of_rooms++;
        room->players = 0;
    }
    struct auth_user *ind = searchn(room->first, user->nickname);
    if (ind !=  NULL){
        if(!ind->logged_on){

            // uzytkownik istnieje offline, mozna zalogowac
            ind->logged_on = true;
            if(send(user->sockkfd, "Auth", 20, 0) == -1) perror("send");
            ind->sockkfd = user->sockkfd;
            ind->address = user->address;
            room->players += 1;
            send(user->sockkfd, &room->players, sizeof(size_t), 0);
            wait_for_player(user->sockkfd, &room->players);
            while(true){
            
                Game_play(ind, room_id);
            }
        }
        else {
            //user online
            if(send(user->sockkfd, "UnAuth", 20, 0) == -1) perror("send");

            return connection_handler((void*) user);
        }
    }
    else {
        
        printf("uzytkownik %s nie istnieje\n", user->nickname);
        if(send(user->sockkfd, "Auth", 20, 0) == -1) perror("send");
        
        user->logged_on = true;
        user->score = 0;
        user->played = 0;

        /*add_user(room->first, room->last, user);*/
        if(room->first == NULL){
            room->first = user;
            room->last = user;
            user->next = NULL;
            //printf("\n room->first is  null\n");
        } else{
            room->last->next = user;
            room->last = user;
        }
        printf("\n Dotychczasowa liczba graczy w pokoju  %lu\n", room->players);
        room->players += 1;
        printf("Liczba graczy w pokoju %lu\n", room->players);
        send(user->sockkfd, &room->players, sizeof(size_t), 0);
        wait_for_player(user->sockkfd, &room->players);
        while(true){
            Game_play(user, room_id);
        }
        room->players -= 1;
    }
    close(user->sockkfd);
    pthread_detach(pthread_self());
    return NULL;
}

void wait_for_player(int socket_id, size_t * player_number){

    while(*player_number == 1){
        sleep(1);
        if(send(socket_id, player_number, sizeof(size_t),0) == -1)
            perror("Error in sending message to klient");
    }
}

int main(int argc, char *argv[]) {


    int new_fd;  
    struct sockaddr_in my_addr;    
    struct sockaddr_in their_addr; 
    socklen_t sin_size;

    if (argc > 2) {
        fprintf(stderr,"Blad uruchomienia: client port_number\n");
        exit(1);
    }

    int port_num;
    if (argc == 1){
        port_num = DEFAULT_PORT;
    } else {
        port_num = atoi(argv[1]);
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    int reuseaddr=1;
    if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&reuseaddr,sizeof(reuseaddr))==-1) {
        perror("setsockopt");
    }


    my_addr.sin_family = AF_INET;         
    my_addr.sin_port = htons(port_num);    
    my_addr.sin_addr.s_addr = INADDR_ANY; 
    
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) \
            == -1) {
        perror("bind");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    signal(SIGPIPE, SIG_IGN);
    read_file_word();

    printf("Serwer rozpoczyna prace ...\n");

    //osobne watki
    while(1) {  
        pthread_t tid;
        sin_size = sizeof(struct sockaddr_in);
        if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, \
                        &sin_size)) == -1) {
            perror("accept");
            continue;
        }
        printf("serwer otrzymal polaczenie od %s\n", inet_ntoa(their_addr.sin_addr));
        auth_user *user = malloc(sizeof(auth_user));
        user->nickname = malloc(20 * sizeof(char));
        user->address = their_addr;
        user->sockkfd = new_fd;
        user->next = NULL;
        user->won = 0;
        user->played = 0;
        user->score = 0;
        user->first_game_score = 0;
        pthread_create(&tid, NULL, &connection_handler, (void*) user);
    }
    return EXIT_SUCCESS;
}
