
#ifndef JOBS_H
#define JOBS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define MAX_JOBS 256

typedef struct {
    int     jid;
    pid_t   pid;
    char    name[256];
    int    active;

} job;

/** Job list management */
void init_jobs(void);
int add_job(char* args[], pid_t p_id);

int del_job(pid_t pid);
int kill_job(char* in);

int suspend_job(pid_t p_id);
//int cont_job(char* jid);

int get_PID(pid_t in_pid);
int is_job(pid_t in);

/** Functions for printing jobs */
int print_suspended_jobs();
void print_all_jobs();

/** RELATED TO CTRL + Z */
int continue_job(char* jid);

/*

I HAVE TO CLEAN UP THE PIPE FILE
HAVE TO CHECK FOR RETURN VALUES AND  STUFF THAT I SKIPPED
PARSE ARGUMENTS CORRECTLY

also i dont think im accounting for max sizes correctly


*/


#endif
