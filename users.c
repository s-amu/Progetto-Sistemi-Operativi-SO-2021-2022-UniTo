#include "users.h"

int main(int argc, char *argv[]) {
	/* Usate anche in users_functions.c */
	extern int SO_USERS_NUM, SO_NODES_NUM, SO_SIM_SEC, SO_BUDGET_INIT, SO_RETRY;
	extern int pos, balance, index_blocks, attempt, semid_ip, *block;
	extern struct p_info *ip;
	extern struct block *ledger;

	/* Usate in users.c */
	int shmid_block, shmid_ip_user, shmid_ledger, j;
	struct sigaction sa;
	sigset_t my_mask;

	/* Seme per la rand() */
	srand(getpid());

	/* Memorizzazione macro da file */
	read_macro(argv[1]);

	/* Segnali */
	sa.sa_handler = handle_signal;
	sa.sa_flags = 0;
	sigemptyset(&my_mask);
	sa.sa_mask = my_mask;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGALRM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL); /* Di default fa break */

	/* Ottengo l'identificatore della memoria per il contatore dei blocchi del libro mastro*/
	if ((shmid_block = shmget(SHMID_BLOCK, sizeof(int), 0)) == -1) {
		fprintf(stderr, "ERROR shmget ");
		TEST_ERROR;
	}

	/* Metto in block l'indirizzo di attacco e lo converto in intero */
	if ((block = (int *)shmat(shmid_block, NULL, 0)) == (void *)-1) {
		fprintf(stderr, "ERROR shmat ");
		TEST_ERROR;
	}

	/* Ottengo l'identificatore della memoria per la struct p_info */
	if ((shmid_ip_user = shmget(getppid(), sizeof(struct p_info) * (SO_USERS_NUM + SO_NODES_NUM), 0)) == -1) {
		fprintf(stderr, "ERROR shmget ");
		TEST_ERROR;
	}

	/* Metto in ip l'indirizzo di attacco e lo converto in struct p_info */
	if ((ip = (struct p_info *)shmat(shmid_ip_user, NULL, 0)) == (void *)-1) {
		fprintf(stderr, "ERROR shmat ");
		TEST_ERROR;
	}

	/* Ottengo l'identificatore della memoria del libro mastro */
	if ((shmid_ledger = shmget(SHMID_LEDGER, sizeof(struct block) * (SO_REGISTRY_SIZE), 0)) == -1) {
		fprintf(stderr, "ERROR shmget ");
		TEST_ERROR;
	}

	/* Metto in ledger l'indirizzo di attacco e lo converto in struct block */
	if ((ledger = (struct block *)shmat(shmid_ledger, NULL, 0)) == (void *)-1) {
		fprintf(stderr, "ERROR shmat ");
		TEST_ERROR;
	}

	/* Metto in semid_ip l'identificatore del set SEMKEY_P_INFO */
	if ((semid_ip = semget(SEMKEY_P_INFO, 0, 0)) == -1) {
		fprintf(stderr, "ERROR semget ");
		TEST_ERROR;
	}

	wait_for_zero(); /* Attende la creazione dei processi */

	/* Leggo la posizione del processo in p_info */
	for (j = 0; j < SO_USERS_NUM; j++) {
		if (ip[j].proc_pid == getpid())
			pos = j;
	}

	balance = SO_BUDGET_INIT; /* Assegno il bilancio iniziale alla variabile */
	index_blocks = 0; /* Fin dove ho letto nel libro mastro (contatore) */
	attempt = 0; /* Numero di tentativi di lancio transazione */

	alarm(rand() % SO_SIM_SEC + 1); /* Dall'inizio della simulazione, passati rand() % SO_SIM_SEC + 1 secondi si lancia un segnale di allarme */

	while (attempt != SO_RETRY) { /* Fino a quando attempt è diverso da SO_RETRY ciclo, altrimenti esco */
		budget_calc(); /* Accedo in lettura al libro mastro per calcolare il bilancio */
		trans_init();  /* Inizializzo transazione */
		trans_send();  /* Invio transazione */
	}

	ip[pos].term = true; /* Aggiorno term in p_info */
	exit(EXIT_FAILURE);
}