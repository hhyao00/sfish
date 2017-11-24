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

#include "../include/jobs.h"
#include "../include/wrappers.h"

#define SYNTAX_ERROR2   "sfish syntax error: %s\n"
#define EXEC_ERROR2     "sfish exec error: %s\n"
#define EXEC_NOT_FOUND2 "sfish: %s: command not found\n"


/*
████████╗██╗  ██╗███████╗    ██████╗ ██╗██████╗ ███████╗██████╗
╚══██╔══╝██║  ██║██╔════╝    ██╔══██╗██║██╔══██╗██╔════╝██╔══██╗
   ██║   ███████║█████╗      ██████╔╝██║██████╔╝█████╗  ██████╔╝
   ██║   ██╔══██║██╔══╝      ██╔═══╝ ██║██╔═══╝ ██╔══╝  ██╔══██╗
   ██║   ██║  ██║███████╗    ██║     ██║██║     ███████╗██║  ██║
   ╚═╝   ╚═╝  ╚═╝╚══════╝    ╚═╝     ╚═╝╚═╝     ╚══════╝╚═╝  ╚═╝
*/


// ======================== PART III: PIPE ======================= //

/** Returns the index after the pipe, or -1 if no pipe */
int chunk_arg( int start, char* dest[], char* src[] ){

    int i;
    for( i = start; src[i] != 0; i++){

        /* The program arg */
        if( strcmp(src[i], "|") == 0 ){
            int k = start;
            int j;
            for( j = 0; j < i-start; j++ ){
                dest[j] = src[k];
                k += 1;
            }
            dest[j] = 0;
            return i+1;
        } // should've exited
    }

    // didn't find pipe. We are at last argument.
    int k = start;
    for( int j = 0; src[j] != 0; j++){
        dest[j] = src[k];
        k += 1;

    }
    dest[k] = NULL;

    return -1; // didn't find pipe. We are at last argument.
}

/** The pipe function */
void io_pipe( char *in_arr[] ){

    /** The number of pipes/ arguments total */
    int arg_count = 0;
    for(int i = 0; in_arr[i] != 0; i++){
        if( strcmp(in_arr[i], "|") == 0 ){ arg_count+=1; }

        if( i > 0 && strcmp(in_arr[i], "|") == 0
            && strcmp(in_arr[i-1], "|") == 0){
            printf(SYNTAX_ERROR2, "error unexpected token");
            //return;
            exit(0);
        }
    }

    if( arg_count == 0 || *in_arr[0] == '|'){
        printf(SYNTAX_ERROR2, "error unexpected token");
        //return;
        exit(0);
    }

    pid_t piper;
    int fd[2];
    int in_arg = 0; // original input is from stdin. Otherwise takes the previous stdout.

    int i;
    int index = 0;

    for( i = 0; i < arg_count+1 ; i++){

        // chunk a command off of in_arr.
        char * args[256] = {0} ;
        index = chunk_arg( index, args, in_arr );
        
        // open the pipe between parent and to be forked child
        if( pipe(fd) < 0 ){ printf("sfish: pipe() failed"); exit(0); } 
        piper = Fork();

        /** The parent will never do anything except take the output of the child
        and feed it into the next argument. It will do this by saving the output of
        the child and letting that be the next input. For the first arg, we start with
        in = 0 = STDIN. For future ones, we read from f[0]. **/
        if( piper == 0 ){

            dup2(in_arg, 0); // let in_arg replace STDIN. In_arg will come from previous loop.
            
            if( index != -1 ){
                // If we aren't at the last argument yet
                dup2(fd[1], 1); // want what we write to replace STDOUT.
                close(fd[0]);   // won't be reading in anymore, already dup2ed necessary to STDIN.
                Execvp( args[0], args );
            }
            else {
                // we're the last argument, so we're only writing to stdout
                close(fd[0]);
                Execvp( args[0], args );
            }
            exit(0);
        } // end child
        else
        {
            close(fd[1]);     // the parent will never write anything
            waitpid(-1, NULL, 0);
            in_arg = fd[0];    // set the in for the next arg.

        }
    } // end for loop
    exit(0);
}

// All of the processes executed in a piped command should have
// the same process group ID and job ID. The process group ID
// should be equal to the process ID of the first child process.

/* The pipe function : EXTRA CREDIT */
int ec_pipe( char *in_arr[] ){

    sigset_t mask, oldmask, mask_all, empty;

    Sigfillset(&mask_all);
    Sigemptyset(&mask);
    Sigemptyset(&empty);
    Sigaddset(&mask, SIGCHLD );

    Sigprocmask(SIG_BLOCK, &mask, &oldmask); // block SIGCHILD.

    pid_t pid = Fork();
    if( pid == 0 ){

        Sigprocmask(SIG_SETMASK, &oldmask, NULL); // unblock for child

        setpgid(0,0);
        tcsetpgrp(STDIN_FILENO, getpid());

        Signal(SIGINT, SIG_DFL);  // CTRL+C
        Signal(SIGTSTP, SIG_DFL); // CTRL+Z
        Signal(SIGTTOU, SIG_DFL);


        // do job.

        io_pipe( in_arr );


    } // end child

    // Block all signals while we add job to queue
    Sigprocmask(SIG_BLOCK, &mask_all, NULL);
    add_job(in_arr, pid);
    Sigprocmask(SIG_SETMASK, &oldmask, NULL);

    sigsuspend(&oldmask);

    // Sets the foreground as our sfish.
    tcsetpgrp(0, getpid());

    return 0; // normal completion
}