#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#define MAX 256

typedef enum {
    _,
    PLAY,
    RANKING,
    QUIT
} Menu;

void Game_menu() 
{
    puts("\n\n""MENU - wybierz numer\n"
                "1 - Graj\n"
                "2 - Tablica wynikow\n"
                "3 - Wyjdz\n\n");
}

void recieve_available_room(int socket_id){
    int16_t number_of_rooms;
    if( recv(socket_id, &number_of_rooms, sizeof(int16_t), 0) < 0)
        printf("Error in recieving ");
    number_of_rooms = htons(number_of_rooms);
    printf("Liczba dostepnych pokoi: %d \n", number_of_rooms);
    for(size_t i = 0 ; i < number_of_rooms; ++i){
        printf("Pokoj nr %lu\n", i);
        printf("Gracze w pokoju : \n");
        int16_t size_of_buffer;
        int nread;
        if( recv(socket_id, &size_of_buffer, sizeof(int16_t), 0) < 0)
            printf("Error in recieving ");
        size_of_buffer = htons(size_of_buffer);
        char buf[size_of_buffer];
        if( (nread = recv(socket_id, &buf, size_of_buffer , 0)) < 0)
            printf("Error in recieving ");
        buf[nread] = '\0';
        printf("\n%s\n", buf);
    }
}


void Board(int sock, char* username)
{
    printf("============================================\n");
    int16_t size_of_buffer;
    if( recv(sock, &size_of_buffer, sizeof(int16_t), 0) < 0)
        printf("Error in receiving ");
    size_of_buffer = htons(size_of_buffer);
    char buf[size_of_buffer];
    int nread;
    if( (nread = recv(sock, &buf, size_of_buffer, 0)) < 0)
      printf("Error in receiving ");
    buf[nread] = '\0';
    printf("\n%s", buf);
    printf("\n\n");
    printf("============================================\n");

}


int menu_input(int sock, char* username) {
		uint16_t converter ;
		int input = 0;
    char in[1];
    do {
        printf("Wybierz opcje 1-3 ->");
        scanf("%s", in);
        input = atoi(in);
    } while (input < 1 || input > 3);
		
        converter  = htons(input);
		if (send(sock, &converter , sizeof(uint16_t), 0) <= 0)
        {
			return QUIT; // input error
		}
		input = ntohs(converter );;
    return input;
}


int sockfd;

int game(int sock, char* username){

	char letters[MAX];
	int n_hang;
	uint16_t converter;
	char word[MAX];
	int gameover = 0;
	int winner = 0;
    char category[MAX];
    
    int nsend;
    if ((nsend= recv(sock, category, MAX, 0)) <= 0  )
        return -1;
    category[nsend] = '\0';
    printf("\nKategoria slowa : %s\n", category);

	while(1){

		//if game over 
		if(recv(sock, &converter, sizeof(uint16_t), 0) <= 0){
			return -1;
		}
		gameover = ntohs(converter);

		//if winner
		if(recv(sock, &converter, sizeof(uint16_t), 0) <= 0){
			return -1;
		}
		winner = ntohs(converter);

		printf("\n============================================\n");
		if(recv(sock, letters, MAX, 0) <= 0){
			return -1;
		}
		printf("\nWykorzystane litery: %s", letters);

		if(recv(sock, &converter, sizeof(uint16_t), 0) <= 0){
			return -1;
		}
		n_hang = ntohs(converter);
		printf("\nDo zalamania sie wisielca pozostalo ruchow: %d\n", n_hang);

		if(recv(sock, word, MAX, 0) <= 0){
			return -1;
		}

		printf("\nSlowo: ");
		for(int i = 0 ; i < strlen(word); i++){
			printf("%c ", word[i]);
		}

		printf("\n");

		if(winner){
			printf("Koniec gry!\n");
			printf("\nGratulacje %s! Wygrales wisielca\n", username);
			break;
		}

		if(gameover){
			printf("Koniec gry!\n");
			printf("\nNiestety %s! Wisielec zostal zalamany, przegrales! Sprobuj jeszcze raz!\n", username);
			break;
		}

		char send_letter;
		printf("Podaj zgadywana litere - ");
		scanf("%s", &send_letter);

		if (send(sock, &send_letter, sizeof(char), 0) <= 0){
			return -1;
		}
	}

	return 0;

}

void Game_play(int sock, char* username) {

    int input;
    do { 
		Game_menu();
        switch ((input = menu_input(sock,username))) {
            case PLAY:
                game(sockfd,username);
                break;
            case RANKING:
                // print_board();
                Board(sockfd,username);
                break;
        }
    } while (input != QUIT);
}

//CTRL+C
void stop(int sigtype) {
    close(sockfd);
    exit(1);
}

int main(int argc, char *argv[]) {
    
    signal(SIGINT, stop);
    struct hostent *he;
    struct sockaddr_in their_addr; 

    if (argc != 3) {
        fprintf(stderr,"blad, aby uruchomic: ./client client_host port\n");
        exit(1);
    }

    if ((he=gethostbyname(argv[1])) == NULL) { 
        herror("gethostbyname");
        exit(1);
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    their_addr.sin_family = AF_INET;      
    their_addr.sin_port = htons(atoi(argv[2]));    
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);
    bzero(&(their_addr.sin_zero), 8);     

    if (connect(sockfd, (struct sockaddr *)&their_addr, \
                sizeof(struct sockaddr)) == -1) {
        perror("connect");
        exit(1);
    }

    printf("Oczekiwanie na odpowiedz serwera...\n");
    printf("============================================\n");
	printf("GRA W WISIELCA - NAUKA ANGIELSKIEGO\n");
    char nickname[20];
    char auth[20];
    int8_t str_len;
    while(true){
        printf("============================================\n");
        printf("\nPodaj swoja nazwe ----> ");
        scanf("%s",nickname);
        if(send(sockfd,nickname,20,0) == -1){
            printf("Error sending username\n");
        }

        uint16_t room_number = 0;
        recieve_available_room(sockfd);
        printf("Chcesz utworzyc nowy pokoj? Podaj liczbe pokojow + 1! \n");
        printf("Podaj numer pokoju do dolaczenia! -----> ");
        scanf("%hu", &room_number);
        uint16_t converter = ntohs(room_number);
        if(send(sockfd, &converter, sizeof(uint16_t), 0) == -1)
            printf("error in sending room number");
        if((str_len = recv(sockfd,auth,20,0)) == -1){
            printf("Error receiving authentication token\n");
        }
        auth[str_len] = '\0';

        if(strcmp(auth, "Auth") != 0){
            printf("Nazwa zajeta! Wybierz inna nazwe\n");
        }
        else break; 
    }
    size_t number_of_players_online;
    recv(sockfd, &number_of_players_online, sizeof(size_t), 0);
    printf("Jestes sam w pokoju, gra zacznie sie gdy dalaczy inny gracz!\n");
    while(number_of_players_online == 1){
        recv(sockfd, &number_of_players_online, sizeof(size_t), 0);
        sleep(1);
    }
    Game_play(sockfd,nickname);

    printf("Disconnected\n");
    close(sockfd);

    return 0;
}

