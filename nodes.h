#ifndef __NODES_H__
#define __NODES_H__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "header.h"

int reserveSem(int semid);
int releaseSem(int semid);
void wait_for_zero();
void set_timestamp();
void one_sec_waited_nodes();
void handle_signal(int sig);
void add_reward_trans();
void read_macro();

#endif