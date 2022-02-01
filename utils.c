#include "utils.h"
extern struct auth_user *auth_user_first;
extern struct auth_user *auth_user_last;
extern struct Hang *Hang_first;
extern struct Hang *Hang_last;
extern struct Leaderboard *Leaderboard_first;
extern struct Leaderboard *Leaderboard_last;
extern char **nickname;
extern char **word;
extern int won;
extern int played;
extern size_t number_of_rooms;

void add_user(auth_user *first,auth_user * last,auth_user *user){
    if (first == NULL)
    {
        printf("dodaje do pierszego elementu \n");
        first = user;
        last = user;
    } else
    {
        last->next = user;
        last = user;
    }
}

struct auth_user* searchn(struct auth_user *ptr, char* nickname)
{
    printf("\n szukanie uzytkownika \n");
    while(ptr != NULL){
        printf("\n user to %s\n", ptr->nickname);
        if(strcmp(ptr->nickname, nickname)== 0){
            return ptr;
        }
        ptr = ptr->next;
    }
    return ptr;
}


//czytanie pliku inputu
void read_file_word() {
    FILE *fp;
    char *words1, buf[100];
    void **pair;
    
    fp = fopen("250words.txt", "r");
    if (fp == NULL) {
        puts("Blad odczytu pliku z haslami");
        exit(EXIT_FAILURE);
    }

    fgets(buf, sizeof buf, fp);
    while (fgets(buf, sizeof buf, fp) != NULL) 
    {                           
        words1 = malloc(10 * sizeof(char));
        pair = malloc(2*sizeof(char*)); //container
        
        sscanf(buf, "%s", words1);
        pair[0] = words1;
        add_words(pair[0]);
    }

    fclose(fp);
}

//dodawanie slow do listy
void add_words(char * word){
    struct Hang * hangman;
    hangman = (struct Hang*)malloc(sizeof(struct Hang));
    hangman->word = word;
    hangman->next = NULL;

    if (Hang_first == NULL) {
        Hang_first = hangman;
        Hang_last = hangman;
    } else {
        Hang_last->next = hangman;
        Hang_last = hangman;
    }

}

//randomowe slowo z listy
char *printRandom(struct Hang *head)
{
    if (head == NULL)
        return(0);

    srand(time(NULL));

    char *result = head->word; 

    struct Hang *current = head;
    int n;
    for (n=2; current!=NULL; n++)
    {
        if (rand() % n == 0)
            result = current->word;

        current = current->next;
    }
    return result;

}


void add_board (char * nickname,int won1, int played1 ){
    struct Leaderboard * board;
    played = played + played1;
    won = won + won1;
    board = (struct Leaderboard*)malloc(sizeof(struct Leaderboard));
    board->players = 0;
    board->next = NULL;

    if (Leaderboard_first == NULL) {
        Leaderboard_first = board;
        Leaderboard_last = board;
    } else {
        Leaderboard_last->next = board;
        Leaderboard_last = board;
    }

}


Leaderboard* create_new_room(uint8_t id){
    if(Leaderboard_first != NULL){
        Leaderboard *new = malloc(sizeof(Leaderboard));
        new->id = id;
        new->next = NULL;
        Leaderboard_last->next = new;
        Leaderboard_last = new;
        new->players = 0;
        return new;
    } else{
        Leaderboard_first = malloc(sizeof(Leaderboard));
        Leaderboard_last = Leaderboard_first;
        Leaderboard_first->id = 0;
        return Leaderboard_first;
    }
}

Leaderboard* search_room(uint8_t id) {
    Leaderboard *ptr = Leaderboard_first;
    while(ptr != NULL){
        if(ptr->id == id)
            return ptr;
        ptr = ptr->next;
    }
    return NULL;
}


void add_to_room(uint8_t id, auth_user * user){
    Leaderboard *ptr = Leaderboard_first;
    while(!(ptr == NULL || ptr->id == id)){
        ptr = ptr->next;
    }
    add_user(ptr->first, ptr->last, user);
}

