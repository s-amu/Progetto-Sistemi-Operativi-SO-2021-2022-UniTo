#include "users.h"

int SO_USERS_NUM, SO_NODES_NUM, SO_SIM_SEC, SO_BUDGET_INIT, SO_MIN_TRANS_GEN_NSEC, SO_MAX_TRANS_GEN_NSEC, 
	SO_REWARD, SO_RETRY, SO_TP_SIZE, SO_MIN_TRANS_PROC_NSEC, SO_MAX_TRANS_PROC_NSEC;
int pos, attempt, balance, index_blocks, semid_ip, msgqid_user, r_node, *block;
struct transaction trans;
struct p_info *ip;
struct block *ledger;
struct message mymsg;
struct sembuf sops;

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
		else if (strcmp(macro, "SO_REWARD") == 0)
			SO_REWARD = val;
		else if (strcmp(macro, "SO_MIN_TRANS_GEN_NSEC") == 0)
			SO_MIN_TRANS_GEN_NSEC = val;
		else if (strcmp(macro, "SO_MAX_TRANS_GEN_NSEC") == 0)
			SO_MAX_TRANS_GEN_NSEC = val;
		else if (strcmp(macro, "SO_RETRY") == 0)
			SO_RETRY = val;
		else if (strcmp(macro, "SO_TP_SIZE") == 0)
			SO_TP_SIZE = val;
		else if (strcmp(macro, "SO_MIN_TRANS_PROC_NSEC") == 0)
			SO_MIN_TRANS_PROC_NSEC = val;
		else if (strcmp(macro, "SO_MAX_TRANS_PROC_NSEC") == 0)
			SO_MAX_TRANS_PROC_NSEC = val;
		else if (strcmp(macro, "SO_SIM_SEC") == 0)
			SO_SIM_SEC = val;
	}

	/* Chiusura file macro.txt */
	if (fclose(apriFile)) {
		fprintf(stderr, "ERROR fclose ");
		TEST_ERROR;
	}
}

/* Attende la creazione dei processi */
void wait_for_zero() {
	/* Decremento di 1 il semaforo */
	sops.sem_flg = 0;
	sops.sem_num = 0;
	sops.sem_op = -1;
	if (semop(semid_ip, &sops, 1) == -1) {
		fprintf(stderr, "ERROR semop ");
		TEST_ERROR;
	}

	/* Wait for zero */
	sops.sem_flg = 0;
	sops.sem_num = 0;
	sops.sem_op = 0;
	if (semop(semid_ip, &sops, 1) == -1) {
		fprintf(stderr, "ERROR semop ");
		TEST_ERROR;
	}
}

/* Calcolo il bilancio dell'utente leggendo nel libro mastro */
void budget_calc() {
	int i;

	if (block[0] > 0) { /* Se il numero di blocchi scritti è > 0 vado avanti */
		while (index_blocks < block[0]) { /* Ciclo per ogni blocco scritto nel libro mastro */
			for (i = 0; i < SO_BLOCK_SIZE - 1; i++) { /* Ciclo tutte le transazioni nel blocco index_blocks */
				if (ledger[index_blocks].array_trans[i].receiver == getpid()) { /* Se la transazione i nel blocco index_blocks è il ricevente allora: */
					balance += ledger[index_blocks].array_trans[i].quantity; /* Aggiungo al suo bilancio la quantità ricevuta */
				}
			}
			index_blocks++; /* Aggiorno l'indice del blocco */
		}
	}
}

/* Inizializzo la transazione da inviare */
void trans_init() {
	trans.timestamp = 0;
	trans.sender = getpid();
	trans.receiver = 0;
	trans.quantity = 0;
	trans.reward = 0;
}

/* Invio transazione */
void trans_send() {
	if (balance >= 2) { /* Imposto transazione */
		set_users_timestamp();										  /* Setta il timestamp della transazione */
		set_users_receiver();										  /* Altro processo utente destinatario a cui inviare il denaro */
		r_node = ip[(rand() % SO_NODES_NUM) + SO_USERS_NUM].proc_pid; /* Altro processo nodo a cui inviare la transazione da processare */
		set_users_quantity();										  /* Intero compreso tra 2 e il suo bilancio */

		/* Metto in msgqid_user l'indirizzo della coda di messaggi a cui inviare la transazione */
		if ((msgqid_user = msgget(r_node, 0)) == -1) {
			fprintf(stderr, "ERROR msgget ");
			TEST_ERROR;
		}

		mymsg.mtype = 1; /* Tipo del messaggio > 0 */
		mymsg.mtext = trans; /* Testo del messaggio */
		if (msgsnd(msgqid_user, &mymsg, sizeof(struct transaction), IPC_NOWAIT) == -1) { /* Msgsnd fallita */
			if (errno != EAGAIN) { /* Errore */
				fprintf(stderr, "ERROR msgsnd ");
				TEST_ERROR;
			}
			/* Se errno == EAGAIN allora la coda è piena */
		} else { /* Msgsnd riuscita */
			balance -= trans.quantity;
			one_sec_waited_users(); /* Attendo intervallo di tempo casuale tra SO_MAX_TRANS_GEN_NSEC e SO_MIN_TRANS_GEN_NSEC */
			attempt = 0;
		}
	} else { /* Se balance < 2: il processo non invia alcuna transazione */
		attempt++;
	}
}

/* Attende un intervallo di nanosecondi tra SO_MIN_TRANS_GEN_NSEC a SO_MAX_TRANS_GEN_NSEC */
void one_sec_waited_users() {
	struct timespec time;
	int random;

	/* Numero casuale da SO_MIN_TRANS_GEN_NSEC a SO_MAX_TRANS_GEN_NSEC */
	random = (rand() % (SO_MAX_TRANS_GEN_NSEC - SO_MIN_TRANS_GEN_NSEC + 1)) + SO_MIN_TRANS_GEN_NSEC;
	time.tv_sec = random / NSEC;
	time.tv_nsec = random % NSEC;
	if (clock_nanosleep(CLOCK_MONOTONIC, 0, &time, NULL) == -1) {
		fprintf(stderr, "ERROR clock_nanosleep ");
		TEST_ERROR;
	}
}

/* Setta il timestamp della transazione */
void set_users_timestamp() {
	struct timespec tp;
	int control;

	if ((control = clock_gettime(CLOCK_REALTIME, &tp)) == -1) {
		fprintf(stderr, "ERROR clock_gettime ");
		TEST_ERROR;
	}

	trans.timestamp = tp.tv_sec * (NSEC) + tp.tv_nsec; /* Secondi attuali in nanosecondi + nanosecondi nel secondo corrente */
}

/* Setta l'utente ricevente della transazione */
void set_users_receiver() {
	int pos_random;

	pos_random = rand() % SO_USERS_NUM; /* Estrae da 0 fino a SO_USERS_NUM */

	/* Ciclo e trovo un altro pos_random fino a quando non ho trovato un utente diverso da se stesso e non terminato */
	while (pos_random == pos || ip[pos_random].term == true) {
		pos_random = rand() % SO_USERS_NUM;
	}

	trans.receiver = ip[pos_random].proc_pid; /* Assegno il ricevente */
}

/* Setta la quantità di denaro da mandare all'utente e il reward al nodo*/
void set_users_quantity() {
	int random;

	random = (rand() % (balance - 1)) + 2; /* Estrae un numero tra 2 e balance. rand() % (massimo - minimo + 1) + minimo */

	if (((SO_REWARD * random) / 100) < 1) { /* Se il reward è minore di 1, gli assegno 1 */
		trans.reward = 1;
	} else {
		trans.reward = (SO_REWARD * random) / 100; /* Assegno la percentuale 'random' di reward */
	}

	trans.quantity = random - trans.reward; /* Assegno la quantità da inviare all'utente */
}

/* Gestione segnali */
void handle_signal(int sig) {
	switch (sig) {
		case SIGTERM:
			shmdt(ip); /* Stacca ip dalla memoria condivisa */
			shmdt(ledger); /* Stacca ledger dalla memoria condivisa */
			exit(EXIT_SUCCESS);
	
		case SIGALRM: /* All'inizio della simulazione, passati rand() % SO_SIM_SEC + 1 secondi si cattura SIGALRM e viene mandata una transazione */
			budget_calc();
			trans_send();
			break;
	
		case SIGINT: /* Il segnale CTRL-C va a tutti i processi del chiamante */
			pause(); /* Il processo è bloccato finchè non riceve un segnale */
			break;
	
		default:
			break;
	}
}