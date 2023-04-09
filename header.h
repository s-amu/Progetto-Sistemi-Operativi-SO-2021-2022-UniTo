#ifndef __HEADER_H__
#define __HEADER_H__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <signal.h>

/* Definizione struttura degli errori (data dal professore) */
#define TEST_ERROR if (errno) { fprintf(stderr, "nel file: %s a linea: %d, con pid %d; ERRORE: %d (%s)\n", \
  __FILE__, __LINE__, getpid(), errno, strerror(errno)); exit(EXIT_FAILURE); }

/* Parametri definiti a tempo di compilazione */
#define SO_REGISTRY_SIZE 1000 /* Numero massimo di blocchi nel libro mastro */
#define SO_BLOCK_SIZE 10 /* Numero di transazioni contenute in un blocco */

#define NSEC 1000000000

#define NODE_SENDER -1

#define MAX_PROC 15

/* Mmoria condivisa */
#define SHMID_LEDGER 76438
#define SHMID_BLOCK 46320

/* Semafori */
#define SEMKEY_P_INFO 85674
#define SEMKEY_LEDGER 22886

/* Struttura di una transazione */
struct transaction {
	long timestamp;
	pid_t sender;
	pid_t receiver;
	int quantity;
	int reward;
};

/* Struttura per avere le informazioni di ogni processo */
struct p_info {
	pid_t proc_pid;
	int proc_balance;
	int pos;
	bool term;
};

/* Struttura di un blocco del libro mastro */
struct block {
	struct transaction array_trans[SO_BLOCK_SIZE]; /* Array di transazioni in un singolo blocco. Un blocco contiene SO_BLOCK_SIZE transazioni */
	int id;
};

/* Struttura dei messaggi */
struct message {
	long mtype;
	struct transaction mtext;
};

#endif