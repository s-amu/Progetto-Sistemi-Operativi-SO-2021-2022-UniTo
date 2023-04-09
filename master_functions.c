#include "master.h"

int SO_USERS_NUM, SO_NODES_NUM, SO_SIM_SEC, SO_BUDGET_INIT;
int users_term, shmid_block, shmid_ip, shmid_ledger, semid_ip, semid_led, msgqid, *block;
struct p_info *ip;
struct block *ledger;
struct msqid_ds buf;

/* Memorizzazione macro da file */
void read_macro(char *argv) {
	char macro[30];
	FILE *apriFile;
	int val;

	/* Apertura file macro.txt */
	if ((apriFile = fopen(argv, "r")) == NULL) {
		fprintf(stderr, "ERROR fopen ");
		TEST_ERROR;
	}

	/* Lettura valori da file macro.txt */
	while (fscanf(apriFile, "%s" "%d", macro, &val) != EOF) {
		if (strcmp(macro, "SO_USERS_NUM") == 0)
			SO_USERS_NUM = val;
		else if (strcmp(macro, "SO_NODES_NUM") == 0)
			SO_NODES_NUM = val;
		else if (strcmp(macro, "SO_BUDGET_INIT") == 0)
			SO_BUDGET_INIT = val;
		else if (strcmp(macro, "SO_SIM_SEC") == 0)
			SO_SIM_SEC = val;
	}

	/* Chiusura file macro.txt */
	if (fclose(apriFile)) {
		fprintf(stderr, "ERROR fclose ");
		TEST_ERROR;
	}
}

/* Attesa di un secondo */
void one_sec_waited_master() {
	struct timespec time;

	time.tv_sec = 1 / NSEC;
	time.tv_nsec = 999999999;
	if (clock_nanosleep(CLOCK_MONOTONIC, 0, &time, NULL) == -1) {
		fprintf(stderr, "ERROR clock_nanosleep ");
		TEST_ERROR;
	}
}

/* Rimozione risorse IPC */
void clear() {
	/* Deallocazione memoria condivisa */
	shmdt(ip);
	shmctl(shmid_ip, IPC_RMID, NULL);
	shmctl(shmid_ledger, IPC_RMID, NULL);
	shmctl(shmid_block, IPC_RMID, NULL);

	/* Deallocazione semafori */
	semctl(semid_ip, 0, IPC_RMID);
	semctl(semid_led, 0, IPC_RMID);
}

/* Stampa solo utenti maggiori e minori */
void print_max_min_users(struct p_info *ptr) {
	int min, max, pos_min, pos_max, i;

	min = ptr[0].proc_balance;
	max = ptr[0].proc_balance;
	pos_min = 0;
	pos_max = 0;

	for (i = 1; i < SO_USERS_NUM; i++) {
		if (ptr[i].proc_balance < min) {
			pos_min = ptr[i].pos;
			min = ptr[i].proc_balance;
		}
		if (ptr[i].proc_balance > max) {
			pos_max = ptr[i].pos;
			max = ptr[i].proc_balance;
		}
	}

	if (ptr[pos_min].term == true) {
		printf("User | PID: %d --> MIN Balance: %d [terminated]\n", ptr[pos_min].proc_pid, min);
	} else {
		printf("User | PID: %d --> MIN Balance: %d \n", ptr[pos_min].proc_pid, min);
	}

	if (ptr[pos_max].term == true) {
		printf("User | PID: %d --> MAX Balance: %d [terminated]\n", ptr[pos_max].proc_pid, max);
	} else {
		printf("User | PID: %d --> MAX Balance: %d \n", ptr[pos_max].proc_pid, max);
	}
}

/* Stampa solo nodi maggiori e minori */
void print_max_min_nodes(struct p_info *ptr) {
	int min, max, pos_min, pos_max, i;

	min = ptr[SO_USERS_NUM].proc_balance;
	max = ptr[SO_USERS_NUM].proc_balance;
	pos_min = SO_USERS_NUM;
	pos_max = SO_USERS_NUM;

	for (i = SO_USERS_NUM + 1; i < SO_USERS_NUM + SO_NODES_NUM; i++) {
		if (ptr[i].proc_balance < min) {
			pos_min = ptr[i].pos;
			min = ptr[i].proc_balance;
		}
		if (ptr[i].proc_balance > max) {
			pos_max = ptr[i].pos;
			max = ptr[i].proc_balance;
		}
	}

	printf("Node | PID: %d --> MIN Balance: %d \n", ptr[pos_min].proc_pid, min);
	printf("Node | PID: %d --> MAX Balance: %d \n", ptr[pos_max].proc_pid, max);
}

/* Calcola i bilanci dei processi leggendo nel libro mastro */
void read_balance_ledger() { /* Eseguita ogni secondo: aggiorno il bilancio (proc_balance) di ogni processo (guarda struttura p_indo nell'header) */
	int i, j, k;

	/* block[0]: contatore di quanti blocchi nel libro mastro sono scritti */
	for (i = 0; i < SO_USERS_NUM; i++) { /* Scandisce tutti i processi utente */
		ip[i].proc_balance = SO_BUDGET_INIT;
		for (j = 0; j < block[0]; j++) { /* Scandisce ogni blocco del libro mastro */
			for (k = 0; k < SO_BLOCK_SIZE; k++) { /* Scandisce ogni singola transazione */
				if (ip[i].proc_pid == ledger[j].array_trans[k].sender) { /* Se il processo i è il mandante della transazione */
					ip[i].proc_balance -= ledger[j].array_trans[k].quantity; /* Bilancio di i meno quantità della transazione inviata */
				}
				if (ip[i].proc_pid == ledger[j].array_trans[k].receiver) { /* Se il processo i è il ricevente della transazione */
					ip[i].proc_balance += ledger[j].array_trans[k].quantity; /* Bilancio di i più quantità della transazione ricevuta */
				}
			}
		}
	}

	/* Aggiorna il bilancio dei nodi con l'ultima transazione del blocco (reward) */
	for (i = SO_USERS_NUM; i < SO_USERS_NUM + SO_NODES_NUM; i++) { /* Scandisce tutti i nodi */
		ip[i].proc_balance = 0;
		for (j = 0; j < block[0]; j++) { /* Scandisce ogni blocco del libro mastro */
			if (ip[i].proc_pid == ledger[j].array_trans[SO_BLOCK_SIZE - 1].receiver) { /* L'ultima transazione in ogni blocco del libro mastro */
				ip[i].proc_balance += ledger[j].array_trans[SO_BLOCK_SIZE - 1].quantity; /* Bilancio di i più quantità della transazione ricevuta */
			}
		}
	}
}

/* Stampa i processi */
void print_proc(struct p_info *ip) {
	int i;

	read_balance_ledger(); /* Leggo il libro mastro e aggiorno p_info */

	if (SO_USERS_NUM > MAX_PROC) {
		if (SO_NODES_NUM > MAX_PROC) {
			/* CASO 1: stampa processi utenti e nodi con maggior e minor budget */
			print_max_min_users(ip);
			print_max_min_nodes(ip);
		} else if (SO_NODES_NUM <= MAX_PROC) {
			/* CASO 2: stampa processi utenti con maggior e minor budget e tutti i nodi */
			print_max_min_users(ip);

			for (i = SO_USERS_NUM; i < SO_USERS_NUM + SO_NODES_NUM; i++) {
				printf("Node | PID: %d -->     Balance: %d \n", ip[i].proc_pid, ip[i].proc_balance);
			}
		}
	} else if (SO_USERS_NUM <= MAX_PROC) {
		if (SO_NODES_NUM > MAX_PROC) {
			/* CASO 3: stampa processi nodo con maggior e minor budget e tutti gli utenti */
			for (i = 0; i < SO_USERS_NUM; i++) {
				if (ip[i].term == true) {
					printf("User | PID: %d -->     Balance: %d [terminated]\n", ip[i].proc_pid, ip[i].proc_balance);
				} else {
					printf("User | PID: %d -->     Balance: %d \n", ip[i].proc_pid, ip[i].proc_balance);
				}
			}
			print_max_min_nodes(ip);
		} else if (SO_NODES_NUM <= MAX_PROC) {
			/* CASO 4: stampa tutti i processi utenti e nodo */
			for (i = 0; i < SO_USERS_NUM; i++) {
				if (ip[i].term == true) {
					printf("User | PID: %d -->     Balance: %d [terminated]\n", ip[i].proc_pid, ip[i].proc_balance);
				} else {
					printf("User | PID: %d -->     Balance: %d \n", ip[i].proc_pid, ip[i].proc_balance);
				}
			}

			for (i = SO_USERS_NUM; i < SO_USERS_NUM + SO_NODES_NUM; i++) {
				printf("Node | PID: %d -->     Balance: %d \n", ip[i].proc_pid, ip[i].proc_balance);
			}
		}
	}
}

/* Stampa la fine della simulazione */
void print_end(char *string) {
	int j;

	printf("\n-*-*-*-*-*- THE SIMULATION ENDS NOW -*-*-*-*-*-\n\n");
	printf("TERMINATION REASON: %s\n", string); /* String: motivo terminazione */
	print_proc(ip);
	printf("\nUsers processes terminated: %d\n", users_term);

	if (block[0] == SO_REGISTRY_SIZE) {
		printf("Maximum capacity of %d blocks reached in the ledger\n\n", SO_REGISTRY_SIZE);
	} else {
		printf("Occupied blocks in the ledger: %d\n\n", block[0]);
	}

	printf("\n\nNUMBER OF TRANSACTIONS IN THE TRANSACTION POOL:\n\n"); /* Lista di transazioni ricevute da processare */

	for (j = SO_USERS_NUM; j < SO_USERS_NUM + SO_NODES_NUM; j++) { /* Cicla tutti i nodi per stampare i messaggi rimanenti nella TP */
		if ((msgqid = msgget(ip[j].proc_pid, 0)) == -1) { /* Metto in msgqid l'attacco della coda di messaggi del processo j */
			fprintf(stderr, "ERROR msgget ");
			TEST_ERROR;
		} 
		if ((msgctl(msgqid, IPC_STAT, &buf)) == -1) { /* Aggiorno buf per vedere quante transazioni sono rimaste al nodo j */
			fprintf(stderr, "ERROR msgctl ");
			TEST_ERROR;
		}
		printf("Node | PID -->  %d    Remaining transaction --> %ld \n", ip[j].proc_pid, buf.msg_qnum); /* Stampa il numero di messaggi relativi al nodo j */
	}
	printf("\n\n");
}

/* Gestione segnali */
void handle_signal(int sig) {
	switch (sig) {
		case SIGALRM: /* Segnale di allarme: tempo scaduto */
			print_end("Seconds of the simulation passed!\n");
			kill(0, SIGTERM);
			break;
	
		case SIGUSR1: /* Segnale ricevuto dai nodi quando il libro mastro è pieno */
			print_end("The maximum ledger size has been reached!\n");
			kill(0, SIGTERM);
			break;
	
		case SIGINT: /* Segnale di terminazione mandato dall'utente (CTRL-C) */
			print_end("Signal CTRL-C received!\n");
			kill(0, SIGTERM);
			break;
	
		case SIGTERM: /* Segnale di fine simulazione */
			waitpid(0, NULL, 0); /* Attesa morte tutti processi */
			clear();
			exit(EXIT_SUCCESS);
			break;
	
		default:
			break;
	}
}