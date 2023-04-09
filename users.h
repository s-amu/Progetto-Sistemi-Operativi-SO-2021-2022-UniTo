#ifndef __USERS_H__
#define __USERS_H__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "header.h"

void handle_signal(int sig);
void budget_calc();
void trans_init();
void trans_send();
void set_users_timestamp();
void set_users_receiver();
void set_users_quantity();
void one_sec_waited_users();
void wait_for_zero();
void read_macro();

#endif