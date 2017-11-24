#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <readline/readline.h>

#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include "../include/sfish.h"
#include "../include/debug.h"
#include "../include/wrappers.h"
#include "../include/jobs.h"

job *jobs[MAX_JOBS];
int count = 0;


/*
     ██╗ ██████╗ ██████╗      ██████╗ ██████╗ ███╗   ██╗████████╗██████╗  ██████╗ ██╗
     ██║██╔═══██╗██╔══██╗    ██╔════╝██╔═══██╗████╗  ██║╚══██╔══╝██╔══██╗██╔═══██╗██║
     ██║██║   ██║██████╔╝    ██║     ██║   ██║██╔██╗ ██║   ██║   ██████╔╝██║   ██║██║
██   ██║██║   ██║██╔══██╗    ██║     ██║   ██║██║╚██╗██║   ██║   ██╔══██╗██║   ██║██║
╚█████╔╝╚██████╔╝██████╔╝    ╚██████╗╚██████╔╝██║ ╚████║   ██║   ██║  ██║╚██████╔╝███████╗
 ╚════╝  ╚═════╝ ╚═════╝      ╚═════╝ ╚═════╝ ╚═╝  ╚═══╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝ ╚══════╝ panel
*/

/** Initialize the job list */
void init_jobs( void ){
    count = 0;
    for( int i = 0 ; i < MAX_JOBS ; i++ ){
        jobs[i] = NULL;
    }
}


/** Returns the index of the job ( JID ) if successful,
otherwise returns -1 and prints ( job list full ). **/
int add_job(char* args[], pid_t p_id){

    if( count == MAX_JOBS ){
        printf("Error: Max number of running processes reached\n");
        return -1;
    }

    int i = 0;
    while( jobs[i] != NULL ){
         i++ ;      // if == null, then found empty slot
    }

    char temp[256];
    strcpy(temp, "");
    for(int j = 0 ; args[j] != '\0'; j++ ){
         strcat(temp, args[j]);
         strcat(temp, " ");
    }

    jobs[i] = malloc(sizeof(job));
    jobs[i]->jid = i;
    jobs[i]->pid = p_id;
    strncpy(jobs[i]->name, temp, 200);

    jobs[i]->active = 1;   // 1 for active.

    count += 1;
    return 0;
}

/** Corresponding to CTRL + C
Returns the pid of the job deleted upon success,
or -1 upon not finding the job.*/
int del_job(pid_t pid){

    int i = 0;
    while( i < MAX_JOBS ){
        // look for the job with the given pid, and "detach" it
        if( jobs[i] != NULL && (jobs[i])-> pid == pid ){
            free(jobs[i]);
            jobs[i] = NULL;
            count -= 1;
            return 0;
        }
        i++;
    }
    return -1;
}

/** Corresponding to  CTRL + Z. Suspend a job.
Returns the pid of the suspended job. Returns -1
if such a pid does not exist.  **/
int suspend_job(pid_t p_id){

    for(int i = 0 ; i < MAX_JOBS; i++ ){
        if( jobs[i] != NULL && jobs[i]->pid == p_id ){
            jobs[i]->active = 0 ;
            return i;
        }
    }
    return -1;
}

/** Corresponding to fg %JID.
Unsuspend a job. This is triggered by the built in fg %JID
Returns the pid of the suspended job. Returns -1
if such a pid does not exist. The input argument is a string.
 **/
int cont_job(char* jid){

    int id = atoi(jid+1); // + 1 to increment past the % sign.

    for(int i = 0 ; i < MAX_JOBS; i++ ){
        if( jobs[i] != NULL && jobs[i]->jid == id ){
            jobs[i]->active = 1;
            return id; // this is the JID.
        }
    }
    return -1;
}

/** Corresponding to kill %JID or kill PID.
if it's the jid (%) well continue, else call del_job with pid.
Returns the pid of the job deleted. **/
int kill_job(char* in){

    if( *(in) == '%' ){
        int jid = atoi(in+1);
        for(int i = 0 ; i < MAX_JOBS; i++ ){
            if( jobs[i] != NULL && jobs[i]->jid == jid ){
                int temp = jobs[i]->pid;
                del_job( jobs[i]->pid );
                return temp;
            }
        }
    }
    else
    {
        int pid = atoi(in);
        del_job(pid);
        return pid;
    }
    return -1;
}


/** Jobs: This command prints a list of the processes
stopped by Ctrl-Z. Each process must have a unique JID
(job ID number) assigned to it by your shell **/
int print_suspended_jobs( void ){

    for(int i = 0 ; i < MAX_JOBS; i++ ){
        if( jobs[i] != NULL && jobs[i]->active == 0 ){
            printf(JOBS_LIST_ITEM, i, jobs[i]->name );
        }
    }
    return -1;
}


void print_all_jobs( void ){
    if( count == 0 ){ return; }
    else {
        for( int i = 0 ; i < MAX_JOBS ; i++ ){
            if(jobs[i] != NULL)
                printf(JOBS_LIST_ITEM, i, jobs[i]->name );
        } // end for loop
    }
}

/* if given a Jid, we want to get the PID */
pid_t get_PID( int j_in ){

    for(int i = 0 ; i < MAX_JOBS; i++ ){
        if( jobs[i] != NULL && jobs[i]->jid == j_in ){
            return jobs[i]->pid ;
        }
    }
    return -1;
}

/* If given pid, want to know if it's a job */
pid_t is_job( int pidin ){

    for(int i = 0 ; i < MAX_JOBS; i++ ){
        if( jobs[i] != NULL && jobs[i]->pid == pidin ){
            return 0 ;
        }
    }
    return -1;
}


/*
 ██████╗████████╗██████╗ ██╗            ██╗       ███████╗
██╔════╝╚══██╔══╝██╔══██╗██║            ██║       ╚══███╔╝
██║        ██║   ██████╔╝██║         ████████╗      ███╔╝
██║        ██║   ██╔══██╗██║         ██╔═██╔═╝     ███╔╝
╚██████╗   ██║   ██║  ██║███████╗    ██████║      ███████╗
 ╚═════╝   ╚═╝   ╚═╝  ╚═╝╚══════╝    ╚═════╝      ╚══════╝

Below are the functions related to CTRL + Z
*/

/** Explicitly check for status updates on process */
int should_run(pid_t pid, int status){

    if( pid == 0 || errno == ECHILD ){
        return -1; // nothing to do
    }

    /* Suspend, CTRL + Z */
    if(WIFSTOPPED(status)){
        suspend_job(pid);
        kill(pid, SIGTSTP);
        return 0;
    }

    /* Exit, CTRL + C */
    if(WIFSIGNALED(status) || WIFEXITED(status)){
        del_job(pid);
        kill(pid, SIGKILL);
        return 0;
    }

    return -1;
}

/** Helper for continue_job, waits for the unsuspended job */
void wait_on_child(int id){
    int status;
    pid_t pid;

    /** Wait on child and see if we should keep waiting */
    do pid = waitpid(-1, &status, WUNTRACED);
    while(
        should_run(pid, status)
    );
}


/** Continue runnin a job fg %JID*/
int continue_job(char* jid){

    // Manage job stuff. Block all signals. Get the id.
    sigset_t mask, prev;
    Sigfillset(&mask);
    Sigprocmask(SIG_BLOCK, &mask, &prev);

    int temp = cont_job(jid);
    if( temp < 0 ){ return -1; } // if no such job
    pid_t id = get_PID(temp);

    Sigprocmask(SIG_SETMASK, &prev, NULL);

    // Attempt to set it as fg
    if( tcsetpgrp(STDIN_FILENO, id) < 0 ){
        printf("Failed to set process group %d as foreground\n", id);
    }
    kill(id, SIGCONT);

    // handle the unsuspended child
    wait_on_child(id);

    // Set sfish as foreground again
    tcsetpgrp(STDIN_FILENO, getpid());

    return 0; // return the pid.
}

