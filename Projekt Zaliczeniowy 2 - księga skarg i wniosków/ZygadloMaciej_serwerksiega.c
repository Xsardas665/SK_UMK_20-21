#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/shm.h>
#include<sys/msg.h>
#include<string.h>
#include<signal.h>
#include<unistd.h>
#include<pwd.h>
#include<sys/sem.h>
#include<sys/ipc.h>
#define MY_MSG_SIZE 150
#define MY_LOG_SIZE 30

int rekord = 0;
int semafor;
key_t shmkey;
int   shmid;

struct my_data {
	int typ;
	char login[MY_LOG_SIZE];
	char txt[MY_MSG_SIZE];
} *shared_data;

/**
 * Metoda Kończąca Pracę Serwera
 */
void sgnhandle(int signal) {
	printf("\n[Serwer]: dostalem SIGINT => koncze i sprzatam...");
	printf(" (odlaczenie: %s, usuniecie: %s, semafory : %s)\n", 
	(shmdt(shared_data) == 0)        ? "OK" : "blad shmdt",
	(shmctl(shmid, IPC_RMID, 0) == 0)? "OK" : "blad shmctl",
	(semctl(semafor, 0, IPC_RMID) == 0)? "OK" : "blad semctl");
	exit(0);
}

/**
 * Zestaw funkcji obsługujących błędy
 */
void semerr() {
	printf("semafory : %s\n", (semctl(semafor, 0, IPC_RMID) == 0)? "OK" : "blad semctl");
}
void dataerr() {
	printf("odlaczenie: %s\n", (shmdt(shared_data) == 0) ? "OK" : "blad shmdt");
}
void shmerr() {
	printf("usuniecie: %s\n", (shmctl(shmid, IPC_RMID, 0) == 0) ? "OK" : "blad shmctl");
}
void komunikatobslugabledow(int err) {
	printf("Error: Wystapil blad o nr: \n%d\nSkontaktuj się z glownym inzynierem \nIt's not a bug, it's a feature!\n", err);
	exit(1);
}
/**
 * Koniec bloku funkcji obsługujących błędy
 */

/**
 * Metoda do odmiany slowa wpis 
 * przy wypisywaniu ilości 
 * wolnych wpisów
 */
void odmiana(int rek) {
	if(rek == 1) {
		printf("wpis...");
	} else if((rek > 20 || rek < 10) && (rek % 10 < 5) && (rek % 10 > 1)){
		printf("wpisy...");
	} else {
		printf("wpisow...");
	}
}
/**
 * Metoda służąca do wypisania zawartości księgi.
 */
void drukowanie(int signal) {
	int i, written = 0;
	for(i = 0; i < rekord; i++) {
		/**
		 * Sprawdzenie czy rekord jest zapisany, 
		 * czyli czy posiada typ różny 
		 * od zera i otwarty semafor.
		 */
		if(shared_data[i].typ != 0 && semctl(semafor, i, GETVAL) != 0) {
			written++;
		}
	}
	/**
	 * Wypisanie komunikatu w przypadku pustej księgi
	 */
	if(written == 0) {
		printf("\nKsiega skarg i wnioskow jest jeszcze pusta\n");
		return;
	}
	printf("\n___________  Ksiega skarg i wnioskow:  ___________\n");
	for(i = 0; i < rekord; i++) {
		/**
		 * Sprawdzenie czy rekord jest zapisany, 
		 * czyli czy posiada typ różny 
		 * od zera i otwarty semafor.
		 */
		if(shared_data[i].typ != 0 && semctl(semafor, i, GETVAL) != 0) {
			/**
			 * Wypisanie błędu w przypadku braku loginu
			 * W przypadku braku błędu wypisanie loginu
			 */
			if((shared_data[i].login[0]) == '\0') {
				printf("Error: Nie wpisano loginu!");
			} else {
				printf("[%s]", shared_data[i].login);
			}
			/**
			 * Wypisanie treści Wpisu
			 */
			printf(": %s\n", shared_data[i].txt);
		}
	}
}

int main(int argc, char * argv[]) {
	int i;
	struct shmid_ds buf;
	if(argc != 2){
		printf("Error: Niepoprawna ilosc argumentow!\n");
		printf("Prosze uzywac: %s\n", argv[0]);
		komunikatobslugabledow(2);
	}
	/**
	 * konwersja ilosci wpisow ze znakow na liczbe
	 */
	sscanf(argv[1], "%d", &rekord); 
	if(rekord < 1 ){
		printf("Error: Za mało miejsca na wpisy!\n");
		komunikatobslugabledow(3);
	}

	/**
	 * Obsługa Sygnałów
	 */
	signal(SIGINT, sgnhandle);
	signal(SIGTSTP, drukowanie);
	
	printf("[Serwer]: ksiega skarg i wnioskow (WARIANT A)\n");
	printf("[Serwer]: Podaj nazwe pliku do wygenerowania klucza\n          Nazwa musi mieć mniej niż 255 znaków : ");
	char filename[256];
	scanf("%s", filename);
	printf("[Serwer]: tworze klucz...");
	/**
	 * Utworzenie klucza na podstawie pliku zadanego argumentem
	 */

	if((shmkey = ftok(filename, 1)) == -1){
		printf("Error: Nie utworzono klucza!\n");
		komunikatobslugabledow(4);
	}
	
	printf(" OK (klucz: %d)\n", shmkey);
	printf("[Serwer]: tworze semafory...");
	
	/**
	 * Tworzenie Semaforów na podstawie wygenerowanego klucza
	 */
	if((semafor = semget(shmkey, rekord, 0666 | IPC_CREAT | IPC_EXCL)) == -1){
		printf("Error: blad tworzenia semaforow!\n");
		komunikatobslugabledow(5);
	}
	
	printf(" OK\n");
	
	for(i = 0; i < rekord; i++){
		/**
		 * Zamknięcie Semaforów do czasu zakończenia uruchamiania serwera
		 */
		if(semctl(semafor, i, SETVAL, 0) == -1){
			printf("Error: blad nadania wartosci semaforowi nr: %i!\n", i + 1);
			semerr();
			komunikatobslugabledow(6);
		}
	}	
	
	printf("[Serwer]: tworze segment pamieci wspolnej dla ksiegi na %d ", rekord);
	odmiana(rekord);
	
	/**
	 * Tworzenie obszaru pamięci współdzielonej
	 */
	if((shmid = shmget(shmkey, rekord * sizeof(struct my_data), 0666 | IPC_CREAT | IPC_EXCL)) == -1){
		printf("Error: blad tworzenia pamieci!\n");
		semerr();
		komunikatobslugabledow(7);
	}
	/**
	 * Sprawdzenie stanu pamięci
	 */
	if(shmctl(shmid, IPC_STAT, &buf) == -1) {
		printf("Error: blad shmctl!\n");
		shmerr();
		semerr();
		komunikatobslugabledow(8);
	}
	
	printf(" OK (id: %d, rozmiar: %zub)\n", shmid, buf.shm_segsz);
	printf("[Serwer]: dolaczam pamiec wspolna...");
	/**
	 * Pobranie adresu pierwszego elementu
	 */
	shared_data = (struct my_data *) shmat(shmid, (void *)0, 0);
	/**
	 * Walidacja wskaznika
	 */
	if(shared_data == (struct my_data *)-1) {
		printf("Error: blad wskaznika na pamiec!\n");
		shmerr();
		semerr();
		komunikatobslugabledow(9);
	}
	printf(" OK (adres: %lX)\n", (long int)shared_data);
	/**
	 * Dla każdego rekordu ustawiam typ 0 aby oznaczyć że jest pusty
	 * Ustawiam domyślny login jako nieakceptowalny
	 * odblokowuję semafor gdy rekord jest gotowy do działania
	 */
	for(i = 0; i < rekord; i++) {
		shared_data[i].typ = 0;
		shared_data[i].login[0] = '\0';
		if(semctl(semafor, i, SETVAL, 1) == -1) {
			printf("Error: blad zwalniania semafora nr: %i!\n", i + 1);
			dataerr();
			shmerr();
			semerr();
			komunikatobslugabledow(10);
		}
	}

	printf("[Serwer]: nacisnij Ctrl^Z by wyswietlic stan ksiegi...\n");
	
	/**
	 * Utrzymywanie procesu serwera
	 */
	while(1) {
		pause();
	}	

	return 0;

}

