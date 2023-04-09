#include "nodes.h"

int SO_USERS_NUM, SO_NODES_NUM, SO_SIM_SEC, SO_BUDGET_INIT, SO_MIN_TRANS_GEN_NSEC, SO_MAX_TRANS_GEN_NSEC,
	SO_REWARD, SO_RETRY, SO_TP_SIZE, SO_MIN_TRANS_PROC_NSEC, SO_MAX_TRANS_PROC_NSEC;
int msgqid_node, semid_led, semid_ip, quantity_reward, j;
bool isDeclared;
struct sembuf sops;
struct block *ledger;
struct transaction tr_reward;
struct block curr_block;
struct msqid_ds buf;
bool isDeclared;

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
	while (fscanf(apriFile, "%s" "%d", macro, &val) != EOF)	{
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
		else if (strcmp(macro, "SO_MIN_TRANS_PROC_NSEC") == 0) {
			isDeclared = true;
			SO_MIN_TRANS_PROC_NSEC = val;
		}
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
	sops.sem_num = 0;
	sops.sem_op = -1;
	sops.sem_flg = 0;
	if (semop(semid_ip, &sops, 1) == -1) {
		fprintf(stderr, "ERROR semop ");
		TEST_ERROR;
	}

	/* Wait for zero */
	sops.sem_num = 0;
	sops.sem_op = 0;
	sops.sem_flg = 0;
	if (semop(semid_ip, &sops, 1) == -1) {
		fprintf(stderr, "ERROR semop ");
		TEST_ERROR;
	}
}

/* Riservo semaforo binario */
int reserveSem(int semid) {
	sops.sem_num = 0;
	sops.sem_op = -1; /* Decremento di 1 il semaforo e lo riservo */
	sops.sem_flg = 0;

	return semop(semid, &sops, 1);
}

/* Rilascio semaforo binario */
int releaseSem(int semid) {
	sops.sem_num = 0;
	sops.sem_op = 1; /* Incremento di 1 il semaforo e lo rilascio */
	sops.sem_flg = 0;

	return semop(semid, &sops, 1);
}

/* Attende un intervallo di nanosecondi tra SO_MIN_TRANS_PROC_NSEC a SO_MAX_TRANS_PROC_NSEC  */
void one_sec_waited_nodes() {
	if (isDeclared) { /* Controllo se SO_MIN_TRANS_PROC_NSEC e SO_MAX_TRANS_PROC_NSEC sono presenti nel file macro.txt */
		struct timespec time;
		int time_range;

		time_range = (rand() % (SO_MAX_TRANS_PROC_NSEC - SO_MIN_TRANS_PROC_NSEC + 1)) + SO_MIN_TRANS_PROC_NSEC;
		time.tv_sec = time_range / NSEC;
		time.tv_nsec = time_range % NSEC;
		if (clock_nanosleep(CLOCK_MONOTONIC, 0, &time, NULL) == -1) {
			fprintf(stderr, "ERROR clock_nanosleep ");
			TEST_ERROR;
		}
	}
}

/* Aggiungo transazione di reward */
void add_reward_trans() {
	set_timestamp();
	tr_reward.sender = NODE_SENDER;
	tr_reward.receiver = getpid();
	tr_reward.quantity = quantity_reward;
	tr_reward.reward = 0;
	curr_block.array_trans[j] = tr_reward; /* Aggiunge nella posizione j del blocco la transazione di reward */
}

/* Setta il timestamp della transazione di reward */
void set_timestamp() {
	struct timespec tp;
	int control;

	if ((control = clock_gettime(CLOCK_REALTIME, &tp)) == -1) {
		fprintf(stderr, "ERROR clock_gettime ");
		TEST_ERROR;
	}

	tr_reward.timestamp = tp.tv_sec * (NSEC) + tp.tv_nsec; /* Secondi attuali in nanosecondi + nanosecondi nel secondo corrente */
}

/* Gestione segnali */
void handle_signal(int sig) {
	switch (sig) {
		case SIGTERM: 
			if ((msgctl(msgqid_node, IPC_STAT, &buf)) == -1) {
				fprintf(stderr, "ERROR msgctl ");
				TEST_ERROR;
			}
			msgctl(msgqid_node, IPC_RMID, NULL);
			shmdt(ledger);
			exit(EXIT_SUCCESS);
	
		case SIGINT: /* Il segnale CTRL-C va a tutti i processi del chiamante */
			pause(); /* Il processo è bloccato finchè non riceve un segnale */
			break;
			
		default:
			break;
	}
}