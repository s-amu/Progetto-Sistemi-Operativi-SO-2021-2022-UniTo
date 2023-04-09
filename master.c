#include "master.h"

int main(int argc, char *argv[]) {
	/* Usate anche in master_functions.c */
	extern int SO_USERS_NUM, SO_NODES_NUM, SO_SIM_SEC, SO_BUDGET_INIT;
	extern int users_term, semid_ip, semid_led, shmid_block, shmid_ip, shmid_ledger, *block;
	extern struct p_info *ip;
	extern struct block *ledger;

	/* Usate in master.c */
	char *arg[] = {"", "macro.txt", NULL};
	int i, j, value, users_alive;
	struct sigaction sa;
	struct sembuf sops;
	sigset_t my_mask;
	
	/* Memorizzazione macro da file */
	read_macro(argv[1]);

	/* Segnali */
	sa.sa_handler = handle_signal;
	sa.sa_flags = 0;
	sigemptyset(&my_mask);
	sa.sa_mask = my_mask;
	sigaction(SIGALRM, &sa, NULL);
	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	/* Creo memoria condivisa per il contatore dei blocchi del libro mastro */
	if ((shmid_block = shmget(SHMID_BLOCK, sizeof(int), IPC_CREAT | IPC_EXCL | 0666)) == -1) {
		fprintf(stderr, "ERROR shmget ");
		TEST_ERROR;
	}

	/* Metto in block l'indirizzo di attacco e lo converto in intero */
	if ((block = (int *)shmat(shmid_block, NULL, 0)) == (void *)-1) {
		fprintf(stderr, "ERROR shmat ");
		TEST_ERROR;
	}

	/* Inizializzo a 0 il contatore nella memoria condivisa. block[0]: conta i blocchi del libro mastro */
	block[0] = 0;

	/* Creo memoria condivisa per le struct p_info */
	if ((shmid_ip = shmget(getpid(), sizeof(struct p_info) * (SO_USERS_NUM + SO_NODES_NUM), IPC_CREAT | IPC_EXCL | 0666)) == -1) {
		fprintf(stderr, "ERROR shmget ");
		TEST_ERROR;
	}

	/* Metto in ip l'indirizzo di attacco e lo converto in struct p_info (lo posso usare come un array) */
	if ((ip = (struct p_info *)shmat(shmid_ip, NULL, 0)) == (void *)-1) {
		fprintf(stderr, "ERROR shmat ");
		TEST_ERROR;
	}

	/* Creo memoria condivisa per il libro mastro */
	if ((shmid_ledger = shmget(SHMID_LEDGER, sizeof(struct block) * (SO_REGISTRY_SIZE), IPC_CREAT | 0666)) == -1) {
		fprintf(stderr, "ERROR shmget ");
		TEST_ERROR;
	}

	/* Metto in ledger l'indirizzo di attacco e lo converto in struct block */
	if ((ledger = (struct block *)shmat(shmid_ledger, NULL, 0)) == (void *)-1) {
		fprintf(stderr, "ERROR shmat ");
		TEST_ERROR;
	}

	/* Creo semaforo per scrivere nel libro mastro */
	if ((semid_led = semget(SEMKEY_LEDGER, 1, IPC_CREAT | IPC_EXCL | 0666)) == -1) {
		fprintf(stderr, "ERROR semget ");
		TEST_ERROR;
	}

	/* Inizializzo semaforo a 1 */
	if (semctl(semid_led, 0, SETVAL, 1) == -1) {
		fprintf(stderr, "ERROR semctl ");
		TEST_ERROR;
	}

	/* Creo semaforo per aspettare la creazione dei processi */
	if ((semid_ip = semget(SEMKEY_P_INFO, 1, IPC_CREAT | 0666)) == -1) {
		fprintf(stderr, "ERROR semget ");
		TEST_ERROR;
	}

	/* Inizializzo semaforo a SO_USERS_NUM + SO_NODES_NUM + 1 */
	if (semctl(semid_ip, 0, SETVAL, (SO_USERS_NUM + SO_NODES_NUM + 1), 0) == -1) {
		fprintf(stderr, "ERROR semctl ");
		TEST_ERROR;
	}

	/* Creazione processi figli users e nodes */
	for (i = 0; i < (SO_USERS_NUM + SO_NODES_NUM); i++) {
		switch (value = fork()) {
			case -1:
				fprintf(stderr, "Error in fork\n");
				exit(EXIT_FAILURE);
	
			case 0:
				if (i < SO_USERS_NUM) {
					ip[i].proc_pid = getpid();
					ip[i].proc_balance = SO_BUDGET_INIT;
					ip[i].pos = i;
					ip[i].term = false;
	
					if (execvp("./users", arg) < 0) {
						fprintf(stderr, "ERROR execvp ");
						TEST_ERROR;
					}
				} else {
					ip[i].proc_pid = getpid();
					ip[i].proc_balance = 0;
					ip[i].pos = i;
					ip[i].term = false;
	
					if (execvp("./nodes", arg) < 0) {
						fprintf(stderr, "ERROR execvp ");
						TEST_ERROR;
					}
				}
	
			default:
				break;
		}
	}

	/* Fine attesa preparazione processi */
	printf("[MASTER] All processes have been created correctly \n");

	sops.sem_num = 0;
	sops.sem_op = -1; /* Decremento di 1 il semaforo del set SEMKEY_P_INFO e parte la simulazione */
	sops.sem_flg = 0;
	if (semop(semid_ip, &sops, 1) == -1) {
		fprintf(stderr, "ERROR semop ");
		TEST_ERROR;
	}

	alarm(SO_SIM_SEC); /* SO_SIM_SEC: durata della simulazione */

	printf("\n-*-*-*-*-*- THE SIMULATION STARTS NOW -*-*-*-*-*-\n");

	while (1) {
		one_sec_waited_master(); /* Attesa di un secondo per la stampa */
		printf("\n");
		users_alive = SO_USERS_NUM;
		users_term = 0;

		for (j = 0; j < (SO_USERS_NUM + SO_NODES_NUM); j++) { /* Aggiorno il numero di users terminati */
			if (ip[j].term == true)
				users_term++;
		}

		users_alive -= users_term;

		if (users_alive == 1) { /* Ultimo processo attivo che non può più mandare transazioni a nessuno */
			users_term++;
			print_end("All users have ended \n");
			kill(0, SIGTERM); /* Termina tutti i processi nello stesso gruppo del chiamante, chiamante incluso */
			exit(EXIT_SUCCESS);
		} else if (users_alive == 0) {
			print_end("All users have ended \n");
			kill(0, SIGTERM); /* Termina tutti i processi nello stesso gruppo del chiamante, chiamante incluso */
			exit(EXIT_SUCCESS);
		}

		print_proc(ip);
		
		printf("\nTerminated Users: %d\t Active users: %d\t Active nodes: %d\t Blocks in the ledger: %d\n\n", users_term, users_alive, SO_NODES_NUM, block[0]);
	}
}