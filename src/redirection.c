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
#include "../include/wrappers.h"


/*
██████╗ ███████╗██████╗ ██╗██████╗ ███████╗ ██████╗████████╗██╗ ██████╗ ███╗   ██╗
██╔══██╗██╔════╝██╔══██╗██║██╔══██╗██╔════╝██╔════╝╚══██╔══╝██║██╔═══██╗████╗  ██║
██████╔╝█████╗  ██║  ██║██║██████╔╝█████╗  ██║        ██║   ██║██║   ██║██╔██╗ ██║
██╔══██╗██╔══╝  ██║  ██║██║██╔══██╗██╔══╝  ██║        ██║   ██║██║   ██║██║╚██╗██║
██║  ██║███████╗██████╔╝██║██║  ██║███████╗╚██████╗   ██║   ██║╚██████╔╝██║ ╚████║
╚═╝  ╚═╝╚══════╝╚═════╝ ╚═╝╚═╝  ╚═╝╚══════╝ ╚═════╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝
*/


// ======================== PART III: REDIRECTION ======================= //

int redirection(char* input, char *in_arr[]){

    /* Invalid syntax, began with > or < or if there's only one arg */
    if( *in_arr[0] == '\0' || in_arr[1] == NULL || *in_arr[0] == '<'){
        printf(SYNTAX_ERROR, "invalid input");
        return -1;
    }

    int fd_in = -1;
    int fd_out = -1;

    /** Separate out the executable program. It will be [0].
    Side note that first argument IS going to be a exe */
    char *prog[1024] = {0};
    for(int i = 0; in_arr[i] != '\0'; i++){

        /* The program arg */
        if( strcmp(in_arr[i], ">") == 0 || strcmp(in_arr[i], "<") == 0 ){
            for(int j = 0; j < i; j++ ){
                prog[j] = in_arr[j];
            }
        }

        /* The "input.txt" arg */
        if( strcmp(in_arr[i], "<") == 0 ){
            /* Only want to see one < */
            if( fd_in != -1 ){ printf(SYNTAX_ERROR, "invalid input"); return -1; }
            /** Try to open the following file */
            int flags = 0;
            if( *in_arr[i] == '<' ){ flags = O_RDONLY; }
            fd_in = open(in_arr[i+1], flags, S_IRUSR | S_IWUSR);
            if( fd_in < 0 ){ printf(SYNTAX_ERROR, "file error"); return -1; }
        }

        /** Found the "output.txt" arg */
        if( strcmp(in_arr[i], ">") == 0 ){
            if( fd_out != -1 ){ printf(SYNTAX_ERROR, "invalid input"); return -1; }

            int flags = O_WRONLY | O_CREAT | O_TRUNC;
            fd_out = open(in_arr[i+1], flags, S_IRUSR | S_IWUSR);
            if( fd_out < 0 ){ printf(SYNTAX_ERROR, "file error."); return -1; }
        }
    } // end for loop

    /** See if we have three arguments */
    int threesome = -1;
    if( fd_in != -1 && fd_out != -1 ){ threesome = 1; }

    /** Dup2 them respectively to prepare for the PROG */
    pid_t io_pid = Fork();
    if( io_pid == 0 ){

        if( threesome < 0 ){
            /* If we are inputting to program */
            if( fd_in > 0 ){
                dup2(fd_in, STDIN_FILENO); // fd replaces stdin
                close(fd_in);
            }
            /* Else outputting program to file */
            else if( fd_out > 0 ){
                dup2(fd_out, STDOUT_FILENO); // output replaces fd
                close(fd_out);
            }
        }
        else if( threesome == 1 ){ /* Both fd_in and fd_out exist. */

            dup2(fd_in, STDIN_FILENO); // replace stdin with fd_in
            dup2(fd_out, STDOUT_FILENO); // replace stdout with fd_out

            close(fd_in);
            close(fd_out);
        }
        // Execute the program
        Execvp(prog[0], prog);
        exit(1);

    }  // end of child.
    else { waitpid(-1, NULL, 0); }
    return 0;
}

