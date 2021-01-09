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
#include <stdbool.h>

int battlefield_my [4][4];
int battlefield_enemy [4][4];
const int woda = 0;
const int t = 1;
const int z = 2;
const int bt = 3;

#define BUFF_SIZE 256
/**
 * W przypadku zmiany portu nalezy
 * dokonać recznej zmiany w funkcji 
 * getaddrinfo w linia 51
 */
#define MY_PORT 16666

struct my_msg //Paczka danych przesylana miedzy aplikacjami
{
	char name[16];
	char text[BUFF_SIZE];
};

struct ship_one{
	char tile[2];
	bool status;
} s1, s2;

struct ship_two{
	char tile_1[2];
	char tile_2[2];
	bool status;
} s3;

void wypisz() {
	printf("Aktualny stan [Moja Plansza]:\n");
	printf("  1 2 3 4 \n");
	for(int i = 0; i < 4; i++) {
		printf("%d ", i+1);
		for (int j = 0; j < 4; j++)
		{
			switch (battlefield_my[i][j])
			{
			case 0:
				printf("  ");
				break;
			case 1:
				printf("X ");
				break;
			case 2:
				printf("Z ");
				break;
			default:
				printf("  ");
				break;
			}
		}
		printf("\n");
	}
	printf("Aktualny stan [Plansza Przeciwnika]:\n");
	printf("  1 2 3 4 \n");
	for(int i = 0; i < 4; i++) {
		printf("%d ", i+1);
		for (int j = 0; j < 4; j++)
		{
			switch (battlefield_enemy[i][j])
			{
			case 0:
				printf("  ");
				break;
			case 1:
				printf("X ");
				break;
			case 2:
				printf("Z ");
				break;
			case 3:
				printf("O");
				break;
			default:
				printf("  ");
				break;
			}
		}
		printf("\n");
	}
}

void komunikatobslugabledow(int err) //Funckaj wyswietlajaca kod bledu zimportowana z mojego poprzedniego projektu
{
	printf("Error: Wystapil blad o nr: \n%d \nPrzerywam dzialanie \nSkontaktuj się z inzynierem projektu\n", err);
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
	/**
	 * Tworzenie Struktury Gniazda Jeżeli dokonano zmiany portu 
	 * prosze zmienić tutaj wartość drugiego argumentu tak aby 
	 * odpowiadała wpisanemu portowi. 
	 */
    resault = getaddrinfo(argv[1], "16666", NULL, &addrinf);
	/**
	 * Jeżeli zwrócona wartość jest różna od 0 oznacza to że wystąpił błąd
	 */
    if( resault != 0)
    {
       printf("getaddrinfo: %s\n", gai_strerror(resault)); 
       komunikatobslugabledow(2);
    }
	temp = (struct sockaddr_in *)(addrinf->ai_addr);
	/**
	 * Ustawienie dla Klienta i serwera kolejno :
	 * IP w wersji 4
	 * Portu
	 * oraz uniwersalnegointerfejsu sieciowego
	 */
    server.sin_family = AF_INET;
    server.sin_port = htons(MY_PORT);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    client.sin_family = AF_INET;
    client.sin_port = htons(MY_PORT);
	/**
	 * Ustawienie Docelowego Adresu IP
	 */
	client.sin_addr = temp->sin_addr;
	/**
	 * Obliczenie długośći adresu dla funkcji recvfrom
	 */
	addrlen = sizeof(client);
	
	/**
	 * Utworzenie Gniazda
	 */
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("Blad tworzenia gniazda \n");
        komunikatobslugabledow(3);
    }
	/**
	 * Przypisanie adresu do gniazda
	 */
    if(bind(sockfd, (struct sockaddr*)&server, sizeof(server)) < 0)
    {
            printf("Blad wiazania! \n");
            close(sockfd);
            komunikatobslugabledow(4);
    }
	/**
	 * Zwolnienie Pamięci
	 */
	freeaddrinfo(addrinf);
	printf("Rozpoczynam grę z %s.\nNapisz <koniec> by zakonczyc grę.\nUstal Połozenie twoich Statków :", inet_ntoa(client.sin_addr));
	for(int i = 0; i < 4; i++) {
		for(int j = 0; j < 4; j++) {
			battlefield_my[i][j] = woda;
			battlefield_enemy[i][j] = woda;
		}
	}
	battlefield_my[1][2] = t;
	battlefield_my[2][3] = z;
	/**
	 * Wiadomość Kontrolna służąca do rozpoznania czy dołączył nowy uzytkownik
	 */
	strcpy(snd.text, "START");
	if(sendto(sockfd, &snd, sizeof(snd), 0,(struct sockaddr *)&client, sizeof(client)) < 0)
	{
		printf("Blad w komunikacji z partnerem\n");
		close(sockfd);
		komunikatobslugabledow(5);
	}
	/**
	 * Utworzenie procesu potomnego odpowiedzialnego za odbieranie i wypisywanie informacji
	 */
    if((CPID = fork()) == 0)
    {
        while(1)
        {	
			/**
			 * Obsługa błędu odbierania wiadomośći
			 */
			if(recvfrom(sockfd, &rcv, sizeof(rcv), 0, (struct sockaddr *)&client, &addrlen) < 0)
			{
				printf("Wystapil blad w odbieraniu informacji! \n");
				komunikatobslugabledow(6);
			}
			/**
			 * Sprawdzenie wiadomości kontrolnej sygnalizującej dołączenie użytkownika
			 */
			if(strcmp(rcv.text,"START") == 0)
			{
				printf("\n[%s (%s) dolaczyl do rozgrywki]\n", rcv.name, inet_ntoa(client.sin_addr));
			} 
			/**
			 * Sprawdzenie wiadomości kontrolnej sygnalizującej odejście użytkownika
			 */
			else if(strcmp(rcv.text,"<koniec>") == 0)
			{
				printf("\n[%s (%s) zakonczyl rozgrywkę]\n", rcv.name, inet_ntoa(client.sin_addr));
       		}
			/**
			 * Ignorowanie wiadomości <wypisz>
			 */
			else if(strcmp(rcv.text,"<wypisz>") == 0)
			{
				printf("");
       		}
			/**
			* Metoda odpowiedzialna za wypisywanie
			*/
			else
			{
				printf("\n[%s (%s)]: %s\n", rcv.name, inet_ntoa(client.sin_addr), rcv.text);
			}
			printf("[%s]> ", snd.name);
			/**
			 * Pozostawanie informacji gdy nadejdą inne
			 */
			fflush(stdout);
		}
		/**
		 * Metoda Sprzątająca
		 */
		close(sockfd);
		exit(0);
    }
	else //Rodzic, odpowiada za wysylanie komunikacji
	{
		while(1)
        {
			printf("[%s] Wprowadź Koordynaty strzału> ", snd.name);
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
			if (strcmp(snd.text, "<wypisz>") == 0) 
			{
				wypisz();
			}
			
        }
	}
    return 0;
}
