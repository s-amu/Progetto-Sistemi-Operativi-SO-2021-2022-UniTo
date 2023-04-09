#ifndef __MASTER_H__
#define __MASTER_H__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "header.h"

void print_proc(struct p_info *ip);
void print_max_min_users(struct p_info *ptr);
void print_max_min_nodes(struct p_info *ptr);
void print_end(char *string);
void read_balance_ledger();
void one_sec_waited_master();
void handle_signal(int sig);
void clear();
void read_macro();

#endif