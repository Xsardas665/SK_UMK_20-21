#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<unistd.h>
#include<sys/sem.h>
#include<signal.h>

/**
 * Wielkość wpisu
 */
#define MY_MSG_SIZE 150

/**
 * Wielkość Loginu
 */
#define MY_LOG_SIZE 30

key_t shmkey;
int   shmid;
int semafor, freee = -1;
struct my_data {
    int  typ;
    char login[MY_LOG_SIZE];
	char txt[MY_MSG_SIZE];
} *shared_data;

size_t bufsize = MY_MSG_SIZE;
size_t bufsizelog = MY_LOG_SIZE;

/**
 * Metoda do wyswietlania błędów
 */
void printErrorMessage(int err) {
	printf("Error: Wystapil blad o nr: %d\nSkonsultuj się z administratorem aplikacji.", err);
	exit(1);
}

void sighandle() {
	printf("\nPrzerywam prace\n");
	if(freee != -1){
		(shared_data + freee)->typ = 0;
		/**
 		 * Otwarcie Semafora z typem 0 aby uniknąć zmarnowania 
 		 * rekordu w księdze w razie przerwania działania programu przed zapisem.
 		 */
		if(semctl(semafor, freee, SETVAL, 1) == -1) {
			printf("Error: semafor - nie odblokowano!\n");
			printErrorMessage(9);
		}
		printf("Odblokowalem semafor i rekord\n");
	exit(0);
	}
}

/**
 * Metoda do odmiany slowa wpis 
 * przy wypisywaniu ilości 
 * wolnych wpisów
 */
void odmiana(int rek, int fr){
	if((rek - fr) == 1) {
		printf("[%d wolny wpis", rek - fr);
	} else if(((rek - fr) < 10 || (rek - fr) > 20) && (rek - fr) % 10 < 5 && (rek - fr) % 10 > 1){
		printf("[%d wolne wpisy", rek - fr);
	} else {
		printf("[%d wolnych wpisow", rek - fr);
	}
}

/**
 * Metoda główna z obsługą podanych argumentów
 */
int main(int argc, char * argv[]) {
	int rekord, n, m = 0, i;
	int wartosc = 0;
	struct shmid_ds buf;
	
	/**
	 * Sprawdzanie ilośći argumentów
	 */
	if(argc != 3) {
		printf("Error: Niepoprawna ilosc argumentow!\n");
		printf("Prosze uzywac: %s [nazwa pliku do generacji klucza] [login]\n", argv[0]);
		printErrorMessage(1);
	}

	/**
	 * Utworzenie pliku na podstawie pliku zadanego argumentem.
	 */
	if((shmkey = ftok(argv[1], 1)) == -1) {
		printf("Error: Nie utworzono klucza!\n");
		printErrorMessage(2);
	}

	/**
	 * Wyświetlenie wiadomości powitalnej.
	 */
	printf("Klient ksiegi skarg i wnioskow wita!\n");
	
	/**
	 * Pobranie adresu pamięci z klucza i próba połączenia z serwerm.
	 */
	if((shmid = shmget(shmkey, 0, 0)) == -1 ) {
		printf("Error: brak dostepu do pamieci lub serwer nie dziala!\n");
		printErrorMessage(3);
	}
	/**
	 * Sprawdzenie Stanu pamięci współdzielonej 
	 */
	if(shmctl(shmid, IPC_STAT, &buf) == -1) {
		printf("Error: brak dostepu do inforamcji o pamieci!\n");
		printErrorMessage(4);
	}
	
	/**
	 * Wyliczenie ilości dostępnych rekordów 
	 */
	rekord = buf.shm_segsz / sizeof(struct my_data);
	
	/**
	 * Podłączenie do semaforów na podstawie klucza.
	 */
	if((semafor = semget(shmkey, 1, 0)) == -1) {
		printf("Error: blad pobierania semaforow!\n");
		printErrorMessage(5);
	}
	
	/**
	 * Pobranie adresu pierwszego elementu
	 */
	shared_data = (struct my_data *) shmat(shmid, (void *)0, 0);
	
	/**
	 * Walidacja wskaźnika
	 */
	if(shared_data == (struct my_data *)-1) {
		printf("Error: blad wskaznika na pamiec!\n");
		printErrorMessage(6);
	}

	/**
	 * Poszukiwania wolnego rekordu
	 */
	for(i = 0; i < rekord; i++) {

		/**
		 * Sprawdzenie czy rekord jest zapisany (posiada typ różny od 0)
		 */
		if((shared_data + i)->typ == 0) {

			/**
			 * Pobieranie wartości semafora z obecnego rekordu.
			 */
			if((wartosc = semctl(semafor, i, GETVAL)) == -1 ) {
				printf("Error: blad pobierania wartosci semafora nr%d!\n", i+1);
				printErrorMessage(7);
			}

			/**
			 * Sprawdzenie czy semafor jest otwarty
			 */
			if(wartosc == 1) {
				freee = i;

				/**
				 * Zamknięcie semafora, zajmując rekord dla wpisu
				 */
				if(semctl(semafor, freee, SETVAL, 0) == -1) {
					printf("Error: blad blokowania semafora nr %d!!!\n", i + 1);
					printErrorMessage(8);
				}
				break;
			}
		}
	}

	/**
	 * Wypisanie komunikatu w przypadku gdy nie ma wolnych rekordów
	 */
	if(freee == -1) {
		printf("Brak wolnych wpisow!\n");
		return 0;
	}
	
	signal(SIGINT, sighandle);

	odmiana(rekord, freee);	
	printf(" (na %d)]\n", rekord);
	printf("Podaj treść twojego wpisu:\n");

	/**
	 * Oznaczenie Rekordu jako zapisanego
	 */
	(shared_data + freee)->typ = 1;

	/**
	 * Dokonanie wpisu treści wiadomości
	 */
	n = read(0, (shared_data + freee)->txt, bufsize);

	/**
	 * Ręczne dodanie znaku końca linii w celu uniknięcia pętli
	 */
	(shared_data + freee)->txt[n-1] = '\0';

	/**
	 * Dokonanie wpisu loginu
	 */
	strcpy((shared_data + freee)->login, argv[2]);

	/**
	 * Ponowne ręczne umieszczenie znaku końca linii
	 */
	(shared_data + freee)->login[m-1] = '\0';
	printf("Dziekuje za dokonanie wpisu do ksiegi\n");

	/**
	 * Odłączenie się od pamięci
	 */
	shmdt(shared_data + freee);

	/**
	 * Otwarcie semafora w celu zasyganlizowania końca pracy
	 */
	if(semctl(semafor, freee, SETVAL, 1) == -1) {
		printf("Error: semafor - nie odblokowano!\n");
		printErrorMessage(9);
	}
	return 0;
}

