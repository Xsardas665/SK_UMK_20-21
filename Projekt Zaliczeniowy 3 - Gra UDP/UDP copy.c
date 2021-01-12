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


int battlefield[4][4];
const int woda = 0;
const int s1 = 1;
const int s2 = 2;
const int sk = 3;
int myturn;

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

void wypisz() {
	printf("Aktualny stan [Moja Plansza]:\n");
	printf("  1 2 3 4 \n");
	for(int i = 0; i < 4; i++) {
		printf("%d ", i+1);
		for (int j = 0; j < 4; j++)
		{
			switch (battlefield[i][j])
			{
			case 0:
				printf("  ");
				break;
			case 3:
				printf("z ");
				break;
			default:
				printf("x ");
				break;
			}
		}
		printf("\n");
	}
}

void komunikatobslugabledow(int err) //Funkcja wyswietlajaca kod bledu
{
	printf("Error: Wystapil blad o nr: \n%d", err);
	exit(1);
}

int toNumber(char a) {
	if (a == 'A' || a == '1'){
		return 0;
	}
	if (a == 'B' || a == '2'){
		return 1;
	}
	if (a == 'C' || a == '3'){
		return 2;
	}
	if (a == 'D' || a == '4'){
		return 3;
	}
	return -1;
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
	
	myturn = 0;
	int field[4][4];

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
	int hits = 0;
	//getships();
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
				myturn = 1;
			} 
			/**
			 * Sprawdzenie wiadomości kontrolnej sygnalizującej odejście użytkownika
			 */
			else if(strcmp(rcv.text,"<koniec>") == 0)
			{
				printf("\n[%s (%s) zakonczyl rozgrywkę]\n", rcv.name, inet_ntoa(client.sin_addr));
       		}
			else if(strcmp(rcv.text,"<KILL1>") == 0)
			{
				printf("\n[Zatopiłeś jednomasztowiec]\n");
       		}
			/**
			 * Ignorowanie wiadomości <wypisz>
			 */
			else if(strcmp(rcv.text,"<wypisz>") == 0){}
			/**
			* Obsluga strzału
			*/
			else{
				printf("[%s Strzela w %s]", rcv.name, rcv.text);
				char x, y;
				x = rcv.text[0];
				y = rcv.text[1];
				if ((x == 'A' || x == 'B' || x == 'C' || x == 'D') && (y == '1' || y == '2' || y == '3' || y == '4')){
					int cord_x = toNumber(x);
					int cord_y = toNumber(y);

					if(battlefield[cord_x][cord_y] == 1) {
						strcpy(snd.text, "<KILL1>");
					} else if (battlefield[cord_x][cord_y] == 2) {
						strcpy(snd.text, "<HIT2>");
					} else {
						strcpy(snd.text, "<LOSE>");
					}

					if (battlefield[cord_x][cord_y] != 0 && battlefield[cord_x][cord_y] != 3){
						battlefield[cord_x][cord_y] = 3;
					}
					if(sendto(sockfd, &snd, sizeof(snd), 0,(struct sockaddr *)&client, (sizeof(client))) < 0) //obsluga bledu wysylania
					{
						printf("Wystapil blad w nadawaniu komunikacji!\n");
						close(sockfd);
						komunikatobslugabledow(7);
					}
				}
				myturn = 1;
			}
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
			if (myturn > 0)
			{
				printf("[%s] Wprowadź Koordynaty strzału> ", snd.name);
			}
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
				if(sendto(sockfd, &snd, sizeof(snd), 0,(struct sockaddr *)&client, (sizeof(client))) < 0) //obsluga bledu wysylania
				{
					printf("Wystapil blad w nadawaniu komunikacji!\n");
					close(sockfd);
					komunikatobslugabledow(7);
				}
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
