#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define BUFF_SIZE 256
#define MY_PORT 6969 //W przypadku zmiany portu nalezy recznie wprowadzic tez zmiane do getaddrinfo, linia 51

struct my_msg //Paczka danych przesylana miedzy aplikacjami
{
	char name[16];
	char text[BUFF_SIZE];
};

void komunikatobslugabledow(int err) //Funckaj wyswietlajaca kod bledu zimportowana z mojego poprzedniego projektu
{
	printf("Error: Wystapil blad o nr: \n%d \nPrzerywam dzialanie \nSkontaktuj się z glownym inzynierem \nIt's not a bug, it's a feature!\n", err);
	exit(1);
}

int main(int argc, char *argv[])
{
	int resault;
    int sockfd;
	struct sockaddr_in server, client, *temp;
    struct addrinfo *addrinf;
	struct my_msg snd, rcv;
	socklen_t addrlen;
	pid_t CPID;
	
	if( argc == 2 ) //gdy nie podano nazwy uzytkownika
	{
		strcpy(snd.name, "NN");
	}
	else if( argc == 3 ) //gdy podano nazwe uzytkownika
	{
		strcpy(snd.name, argv[2]);  //przypisanie nazwy
	}
	else
	{
		printf("Nieprawidlowa ilosc argumentow. Prawidlowo: ./program IP(osoby z ktora chcesz sie porozumiec) nazwauzytkownika(opcjonalnie)\n");
		komunikatobslugabledow(1);
	}
    resault = getaddrinfo(argv[1], "6969", NULL, &addrinf); //Tworzenie struktury gniazda
    if( resault != 0) //Jesli funkcja nie zwroci 0 to wystapil blad
    {
       printf("getaddrinfo: %s\n", gai_strerror(resault)); 
       komunikatobslugabledow(2);
    }
	temp = (struct sockaddr_in *)(addrinf->ai_addr);	
    server.sin_family = AF_INET; //ustawiam dla klienta i serwera koljno IP jako ver 4, port oraz uniwersalny interface
    server.sin_port = htons(MY_PORT);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    client.sin_family = AF_INET;
    client.sin_port = htons(MY_PORT);
	client.sin_addr = temp->sin_addr; //docelowe IP
	addrlen = sizeof(client);	//dlugosc adresu dla rcvfrom
	
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) //Tworze gniazdo
    {
        printf("Blad tworzenia gniazda \n");
        komunikatobslugabledow(3);
    }

    if(bind(sockfd, (struct sockaddr*)&server, sizeof(server)) < 0) //Przypisuje gniazdu adres
    {
            printf("Blad wiazania! \n");
            close(sockfd);
            komunikatobslugabledow(4);
    } 
	freeaddrinfo(addrinf); //Zwalniam pamiec
	printf("Rozpoczynam czat z %s. Napisz <koniec> by zakonczyc czat.\n", inet_ntoa(client.sin_addr));
	strcpy(snd.text, "START"); //Przypisywanie wiadomosci kontrolnej aby program rozpoznal ze dolaczyl nowy uzytkownik
	if(sendto(sockfd, &snd, sizeof(snd), 0,(struct sockaddr *)&client, sizeof(client)) < 0)
	{
		printf("Blad w komunikacji z partnerem\n");
		close(sockfd);
		komunikatobslugabledow(5);
	}	
    if((CPID = fork()) == 0) //Tworze potomka, odpowiedzialny za odbieranie i wypisywanie wiadomosci
    {
        while(1)
        {
			if(recvfrom(sockfd, &rcv, sizeof(rcv), 0, (struct sockaddr *)&client, &addrlen) < 0) //obsluga bledu odbierania wiadomosci
			{
				printf("Wystapil blad w odbieraniu komunikacji! \n");
				komunikatobslugabledow(6);
			}
			if(strcmp(rcv.text,"START") == 0) //sprawdzenie wiadomosci kontrolnej sygnalizujacej dolaczenie do czatu
			{
				printf("\n[%s (%s) dolaczyl do rozmowy]\n", rcv.name, inet_ntoa(client.sin_addr));
			} 
			else if(strcmp(rcv.text,"<koniec>") == 0) // sprawdzenie wiadomosci kontrolnej sygnalizujacej rozlaczenie czatu
			{
				printf("\n[%s (%s) zakonczyl rozmowe]\n", rcv.name, inet_ntoa(client.sin_addr));
       		}
			else //wypisywanie
			{
				printf("\n[%s (%s)]: %s\n", rcv.name, inet_ntoa(client.sin_addr), rcv.text);
			}
			printf("[%s]> ", snd.name);
			fflush(stdout); //trick z zajec aby uzyskac efekt czatu w konsoli (nie trace wiadmosci pisanej kiedy przyjdzie inna)
		}
		close(sockfd); //sprzatanie
		exit(0);
    }
	else //Rodzic, odpowiada za wysylanie komunikacji
	{
		while(1)
        {
			printf("[%s]> ", snd.name);
			fgets(snd.text, BUFF_SIZE, stdin);
			snd.text[strlen(snd.text)-1] = '\0'; //wstawianie znaku konca linii do wiadomosci
			if(sendto(sockfd, &snd, sizeof(snd), 0,(struct sockaddr *)&client, (sizeof(client))) < 0) //obsluga bledu wysylania
			{
				printf("Wystapil blad w nadawaniu komunikacji!\n");
				close(sockfd);
				komunikatobslugabledow(7);
			}
			if(strcmp(snd.text,"<koniec>") == 0) //wykrycie komendy konczacej
			{
				kill(CPID, SIGINT); //zabijam potomka
				close(sockfd); //sprzatam
				return 0;
			}
        }
	}
    return 0;
}
