#include "nodes.h"

int main(int argc, char *argv[]) {
	/* Usate anche in nodes_functions.c */
	extern int SO_TP_SIZE;
	extern int msgqid_node, semid_led, semid_ip, quantity_reward, j;
	extern bool isDeclared;
	extern struct block *ledger;
	extern struct block curr_block;

	/* Usate in nodes.c */
	int shmid_block, shmid_ledger, *block;
	struct message mymsg;
	struct msqid_ds buf;
	struct sigaction sa;
	sigset_t my_mask;

	/* True se SO_MIN_TRANS_PROC_NSEC e SO_MAX_TRANS_PROC_NSEC sono presenti in macro.txt, false altrimenti */
	isDeclared = false;

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
	sigaction(SIGINT, &sa, NULL); /* Di default fa break */

	/* Creo la coda di messaggi e l'assegno a msgqid_node */
	if ((msgqid_node = msgget(getpid(), IPC_CREAT | 0666)) == -1) {
		fprintf(stderr, "ERROR msgget ");
		TEST_ERROR;
	}

	/* Copio la struttura di msqid_ds nel buffer puntato da buf (inizializzo) */
	if ((msgctl(msgqid_node, IPC_STAT, &buf)) == -1) {
		fprintf(stderr, "ERROR msgctl ");
		TEST_ERROR;
	}

	/* Setto la dimensione di byte massimi nella coda di messaggi */
	buf.msg_qbytes = SO_TP_SIZE * sizeof(struct transaction);

	/* Setto la coda di messaggi con la dimensione massima di byte */
	if ((msgctl(msgqid_node, IPC_SET, &buf)) == -1) {
		fprintf(stderr, "ERROR msgctl ");
		TEST_ERROR;
	}

	/* Ottengo l'identificatore della memoria per il contatore dei blocchi del libro mastro */
	if ((shmid_block = shmget(SHMID_BLOCK, sizeof(int), 0)) == -1) {
		fprintf(stderr, "ERROR shmget ");
		TEST_ERROR;
	}

	/* Metto in block l'indirizzo di attacco e lo converto in intero */
	if ((block = (int *)shmat(shmid_block, NULL, 0)) == (void *)-1) {
		fprintf(stderr, "ERROR shmat ");
		TEST_ERROR;
	}

	/* Ottengo l'identificatore della memoria del libro mastro */
	if ((shmid_ledger = shmget(SHMID_LEDGER, sizeof(struct block) * (SO_REGISTRY_SIZE), 0)) == -1) {
		fprintf(stderr, "ERROR shmget ");
		TEST_ERROR;
	}

	/* Metto in ledger l'indirizzo di attacco e lo converto in struct di block */
	if ((ledger = (struct block *)shmat(shmid_ledger, NULL, 0)) == (void *)-1) {
		fprintf(stderr, "ERROR shmat ");
		TEST_ERROR;
	}

	/* Metto in semid_led l'identificatore del set SEMKEY_LEDGER */
	if ((semid_led = semget(SEMKEY_LEDGER, 0, 0)) == -1) {
		fprintf(stderr, "ERROR semget ");
		TEST_ERROR;
	}

	/* Metto in semid_ip l'identificatore del set SEMKEY_P_INFO */
	if ((semid_ip = semget(SEMKEY_P_INFO, 0, 0)) == -1) {
		fprintf(stderr, "ERROR semget ");
		TEST_ERROR;
	}

	/* Attende la creazione dei processi */
	wait_for_zero();

	while (1) {
		quantity_reward = 0;
		if ((msgctl(msgqid_node, IPC_STAT, &buf)) == -1) { /* Aggiorno buf per vedere quanti messaggi ci sono in coda */
			fprintf(stderr, "ERROR msgctl ");
			TEST_ERROR;
		}

		if (buf.msg_qnum >= SO_BLOCK_SIZE - 1) { /* Se ci sono abbastanza messaggi creo il blocco (SO_BLOCK_SIZE - 1 transazioni ricevute da utenti) */
			for (j = 0; j < SO_BLOCK_SIZE - 1; j++) { /* Leggo SO_BLOCK_SIZE - 1 transazioni dalla coda e le inserisco nel blocco */
				if (msgrcv(msgqid_node, &mymsg, sizeof(struct transaction), 0, 0) == -1) {
					fprintf(stderr, "ERROR msgrcv ");
					TEST_ERROR;
				} /* Riempio il blocco di transazioni */

				quantity_reward += mymsg.mtext.reward;	 /* Sommo la quantità di reward accedendo al campo reward della transaction mtext */
				curr_block.array_trans[j] = mymsg.mtext; /* Aggiunge nella posizione j del blocco la transazione appena letta dalla coda */
			}

			/* Nell'ultima posizione del blocco aggiungo la transazione di reward (che è l'ultima della coda di messaggi ad entrare nel blocco) */
			add_reward_trans();

			/* Nessun altro può scrivere da questo momento sul libro mastro */
			if (reserveSem(semid_led) == -1) { /* Sezione di ingresso */
				fprintf(stderr, "ERROR reserveSem ");
				TEST_ERROR;
			}

			if (block[0] < SO_REGISTRY_SIZE) { /* Sezione critica: scrittura del blocco di transazioni nel libro mastro */
				curr_block.id = block[0];	   /* Aggiorno id blocco con il numero di blocchi correnti */
				one_sec_waited_nodes();		   /* Simulo attesa elaborazione blocco */
				ledger[block[0]] = curr_block; /* Scrivo tutto il blocco nel libro mastro */
				block[0]++;					   /* Incremento numero effettivo di blocchi scritti nel libro mastro */

				if (block[0] == SO_REGISTRY_SIZE) {
					kill(getppid(), SIGUSR1); /* Spazio esaurito quindi avviso il master */
					pause();				  /* Metto in pausa il processo e aspetto segnale SIGTERM */
				}
			}

			/* Rilascia il semaforo, altri processi possono scrivere sul libro mastro */
			if (releaseSem(semid_led) == -1) { /* Sezione di uscita */
				fprintf(stderr, "ERROR releaseSem ");
				TEST_ERROR;
			}
		}
	}
}