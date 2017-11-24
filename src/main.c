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
#include "../include/jobs.h"
#include "../include/wrappers.h"

char *cwd_buf;  // buf for the cwd (pwd)
size_t cwd_len; // length for cwd_buf
char *prompt;   // prompt for path buffer thing

char *io_buf[] = {">", "<", "|", 0 };

char *prompter = PROMPT;

// ---------------- HELPER FUNCTIONS NEEDED FOR MAIN --------------------- //
          

/** Child handler ; corresponds to Signal(SIGCHLD, child_handler); */
void child_handler(int sig){

    int olderrno = errno;
    sigset_t mask_all, prev_all;

    pid_t cpid;
    int status;

    while((cpid = waitpid(-1, &status, WUNTRACED | WNOHANG)) > 0){ //note

        if( is_job(cpid) == -1 ){ return; } // not pid?

        Sigfillset(&mask_all);
        Sigprocmask(SIG_BLOCK, &mask_all, &prev_all); // block everything while we delete

        if(WIFSTOPPED(status)){
            //printf("STOP caught from: %d by %d\n", cpid, WSTOPSIG(status));
            suspend_job(cpid);

        }
        if( WIFSIGNALED(status)){
            //printf("DELETE/term with sig: %d\n", cpid);
            del_job( cpid );
        }
       if( WIFEXITED(status)){
            //printf("WIFEXITED caught from: %d\n", cpid);
            del_job( cpid );
        }

        Sigprocmask(SIG_SETMASK, &prev_all, NULL);
    }

    if( cpid < 0 && errno != ECHILD ){
        printf("Child error\n");
    }

    errno = olderrno;
}


// ========================= CONFIGURE PROMPT ========================= //
/** Configure prompt */
void config_prompt(void){

    int m = 1;
    cwd_buf = Malloc((size_t) m*CWD_BUF);

    if( getcwd( (void*) cwd_buf, m*CWD_BUF) != NULL ){

        /** If we need a bigger buffer */
        while( errno == ERANGE ){
            errno = 0;
            m += 1;
            cwd_buf = realloc(cwd_buf, (size_t) m*CWD_BUF);
            if( cwd_buf == NULL ){ printf("sfish realloc error: Realloc failed. "); }
        }
        prompt = Malloc(m*CWD_BUF);

        char *home_dir = getenv("HOME");
        if(strcmp(cwd_buf, home_dir) == 0){
            /* no need for ~ in prompt */
            strcpy(prompt, home_dir);

        } else {
            /* want ~ to replace home dir */
            int offset = strlen(home_dir); // inc. past home dir.
            strcpy(prompt, "~");
            strcpy(prompt+1, cwd_buf+offset);
        }
    }
    cwd_len = m*CWD_BUF;
}


// ========================= PARSE INPUTS ========================= //

/** Tokenize the string with parameter delim. Returns arg count on success **/
int tokenize_args(char* input, char *in_arr[], char* delim){

    if( strlen(input) > ARGS_BUF_LEN ){ return -1; } // exceeded maximum buffer

    int total_size = ARGS_BUF_LEN;
    char *inputs = strtok(input, delim);
    int i = 0;
    int count = 0;

    while(inputs!=NULL){

        in_arr[i] = inputs;
        total_size -= 1; // pointer count

        if( total_size <= 0 ){ return -1; } // exceeded maximum buffer
        i++; count ++;
        inputs = strtok(NULL, delim);

    }
    in_arr[i] = 0;
    return count;
}



/* Tokenize and keep redirection symbols. io_buff contains: '>', '<', '|'
perm[] will store solely arguments, and arr[]  will be used in execution. */
int tokenize_keep_delim(char* input, char *arr[], char *perm[]){

    if( strlen(input) > ARGS_BUF_LEN ){ return -1; }

    // if this string has no delimiters, normal tokenizing
    if( strchr(input, '<') == NULL && strchr(input, '>') == NULL && strchr(input, '|') == NULL ){
        return tokenize_args(input, arr, "\t\n ");
    }

    char temp_str[1024] = {0};
    char *temp[1024] = {0};
    strncpy(temp_str, input, strlen(input));     // save a copy of the string
    tokenize_args(temp_str, temp, "\t\n ");

    // make the perm array that only contains arguments
    tokenize_args(input, perm, "\t\n ><|");

    int p_indx = 0;
    int a_indx = 0;
    int t_indx = 0;

    // go through all the chunks of arguments that are separated by white space.
    for( t_indx = 0; temp[t_indx] != 0; t_indx++){
        char * str = temp[t_indx];

        // if this chunk of string doesn't have a redirection char then
        if( strchr(str, '<') == NULL && strchr(str, '>') == NULL && strchr(str, '|') == NULL ){
            arr[a_indx] = perm[p_indx];
            a_indx++;
            p_indx++;
            continue;
        }

        // go through that individual string
        for( int c_ptr = 0; *(str + c_ptr) != 0; c_ptr++ ){
            char c = *(str+c_ptr);

            // when encounter a delimiter that we want to keep,
            if( c == '>' || c == '<' || c == '|' ){

                if( c_ptr != 0 ){
                    arr[a_indx] = perm[p_indx];
                    a_indx++;
                    p_indx++;
                }

                // add the appropriate delimiter
                if( c == '>' ){ arr[a_indx] = io_buf[0]; }
                else if( c == '<' ){ arr[a_indx] = io_buf[1]; }
                else if( c == '|' ){ arr[a_indx] = io_buf[2]; }
                a_indx++;

                // if the delimiter is the last char of the string,
                // leave the for loop and go back to the top
                 if( *(str + c_ptr + 1) == 0){
                    break;
                 }
            } // end if for <, > ,|

            // if there's a last argument, add manually.
            if( *(str + c_ptr + 1) == 0){
                arr[a_indx] = perm[p_indx];
                a_indx++;
                p_indx++;
             }
        } // end forloop for one string

    } // end main for loop

     return 0;
}

// --------------------- end of helpers for main  --------------------- //


/*

███╗   ███╗ █████╗ ██╗███╗   ██╗
████╗ ████║██╔══██╗██║████╗  ██║
██╔████╔██║███████║██║██╔██╗ ██║
██║╚██╔╝██║██╔══██║██║██║╚██╗██║
██║ ╚═╝ ██║██║  ██║██║██║ ╚████║
╚═╝     ╚═╝╚═╝  ╚═╝╚═╝╚═╝  ╚═══╝

*/


int main(int argc, char *argv[], char* envp[]) {
    char* input;
    bool exited = false;

    /** Ignore signals to somewhat simulate bash */
    Signal(SIGINT, SIG_IGN);
    Signal(SIGTSTP, SIG_IGN);
    Signal(SIGTTOU, SIG_IGN);
    Signal(SIGCHLD, child_handler);
    init_jobs(); // job list that lives forever.

    if(!isatty(STDIN_FILENO)) {
        // If your shell is reading from a piped file
        // Don't have readline write anything to that file.
        // Such as the prompt or "user input"
        if((rl_outstream = fopen("/dev/null", "w")) == NULL){
            perror("Failed trying to open DEVNULL");
            exit(EXIT_FAILURE);
        }
    }

    // -------- MAIN LOOP FOR SFISH -------- //
    do {
        /** Configure the prompt and ask for input */
        config_prompt();

        /* print prompt if we're getting input from terminal. */
        if( isatty(STDIN_FILENO) ){
            printf(prompter, prompt);
        }

        input = readline(" ");

        // If EOF is read (aka ^D) readline returns NULL
        if(input == NULL) { continue; }

        // Tokenize the string and put it into in_arr
        char *in_arr[1024] = {0};
        char *perm[1024] = {0};

        /** Check if we'll be doing redirection */
        if( strchr(input, '>') != NULL || strchr(input, '<') != NULL ){
            tokenize_keep_delim(input, in_arr, perm);
            redirection( input, in_arr);
            goto CONTINUE;
        }
        else if (strchr(input, '|') > 0 ){
            tokenize_keep_delim(input, in_arr, perm);
            ec_pipe( in_arr );
            goto CONTINUE;
        }

        /** No special parsing for non redirection */
        if( tokenize_args(input, in_arr, "\n\t ") < 0 ){
            printf("Max buffer length exceeded\n");
            continue;
        }

        // write(1, "\e[s", strlen("\e[s"));
        // write(1, "\e[20;10H", strlen("\e[20;10H"));
        // write(1, "SomeText", strlen("SomeText"));
        // write(1, "\e[u", strlen("\e[u"));

        /** Check if input args are built ins. */
        int ret = is_builtins(input, in_arr);
        if( ret == 1 ){ exited = 1; goto CONTINUE; } // detected "exit"
        else if( ret == 0 ){ goto CONTINUE ; }       // input was a built in.

        // If it's not a built in, then it may be an exe
        ret = do_exe(in_arr);
        if( ret == 0 ){ goto CONTINUE; }

        // Free malloced spaces
        CONTINUE:
            free(input);
            free(prompt);
            free(cwd_buf);
    } while(!exited);

    debug("%s", "user entered 'exit'");
    return EXIT_SUCCESS;
}



/*
███████╗██╗  ██╗███████╗ ██████╗██╗   ██╗████████╗ █████╗ ██████╗ ██╗     ███████╗███████╗
██╔════╝╚██╗██╔╝██╔════╝██╔════╝██║   ██║╚══██╔══╝██╔══██╗██╔══██╗██║     ██╔════╝██╔════╝
█████╗   ╚███╔╝ █████╗  ██║     ██║   ██║   ██║   ███████║██████╔╝██║     █████╗  ███████╗
██╔══╝   ██╔██╗ ██╔══╝  ██║     ██║   ██║   ██║   ██╔══██║██╔══██╗██║     ██╔══╝  ╚════██║
███████╗██╔╝ ██╗███████╗╚██████╗╚██████╔╝   ██║   ██║  ██║██████╔╝███████╗███████╗███████║
╚══════╝╚═╝  ╚═╝╚══════╝ ╚═════╝ ╚═════╝    ╚═╝   ╚═╝  ╚═╝╚═════╝ ╚══════╝╚══════╝╚══════╝
*/


/** Fork() the children to execute executable files. */
int do_exe(char *in_arr[]){
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

        if( execvp(in_arr[0], in_arr) < 0 ){

            if(errno == ENOENT){
                printf(EXEC_NOT_FOUND, in_arr[0]);
            } else {
                printf(EXEC_ERROR, in_arr[0]);
            }
            tcsetpgrp(0, getppid());
            exit(1);
        }
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





/*
██████╗ ██╗   ██╗██╗██╗  ████████╗██╗███╗   ██╗███████╗
██╔══██╗██║   ██║██║██║  ╚══██╔══╝██║████╗  ██║██╔════╝
██████╔╝██║   ██║██║██║     ██║   ██║██╔██╗ ██║███████╗
██╔══██╗██║   ██║██║██║     ██║   ██║██║╚██╗██║╚════██║
██████╔╝╚██████╔╝██║███████╗██║   ██║██║ ╚████║███████║
╚═════╝  ╚═════╝ ╚═╝╚══════╝╚═╝   ╚═╝╚═╝  ╚═══╝╚══════╝
*/

// ========================= FUNCTIONS FOR BUILTINS ========================= //


/** The color builtin for the Extra Credit. */
int color_builtin( char* in_arr[] ){

    char *col = in_arr[1]; // already know [1] exists

    if( strcmp(col, "RED") == 0 ){ prompter = RED; return 0; }
    else if( strcmp(col, "GRN") == 0 ){ prompter = GRN; return 0; }
    else if( strcmp(col, "YEL") == 0 ){ prompter = YEL; return 0; }
    else if( strcmp(col, "BLU") == 0 ){ prompter = BLU; return 0; }
    else if( strcmp(col, "MAG") == 0 ){ prompter = MAG; return 0; }
    else if( strcmp(col, "CYN") == 0 ){ prompter = CYN; return 0; }
    else if( strcmp(col, "WHT") == 0 ){ prompter = WHT; return 0; }
    else if( strcmp(col, "BWN") == 0 ){ prompter = BWN; return 0; }
    else{
        printf(BUILTIN_ERROR, COLOR_USAGE);
        return 0;
    }
}

/** Save the env variable cwd as OLDPWD. */
int save_cwd(void){

    int m = 1;
    cwd_buf = Malloc((size_t) m*CWD_BUF);

    if( getcwd( (void*) cwd_buf, m*CWD_BUF ) != NULL ){
        while( errno == ERANGE ){
            errno = 0;
            m += 1;
            cwd_buf = realloc(cwd_buf, (size_t) m*CWD_BUF);
        }
        cwd_len = m*CWD_BUF; // save as global bc I suck.

        if( setenv("OLDPWD", cwd_buf, 1) == 0 ){
            return 0;
        }
    }
    return -1;
}

/**
See if the input argument in_arrp[] is a built in.
Returns 1 if exit, NULL for normal built in, and -1 for no built in.
*/
int is_builtins(char* input, char* in_arr[]){


    //check if it's a color change request first.
    if( in_arr[0] != 0 && strcmp(in_arr[0], "color") == 0 ){

        // if not enough arguments
        if( in_arr[1] == NULL || in_arr[1] == 0 ){
            printf(BUILTIN_ERROR, COLOR_USAGE);
            return 0;
        } else {
            return color_builtin( in_arr );
        }
    } // end color_builtin


    /* A mask that blocks all signals */
    sigset_t mask, prev;
    Sigfillset(&mask);

    /* Go through and look through the tokenized strings */
    for(int j = 0 ; in_arr[j] != 0; j++){

        /* Fork for builts "help" and "pwd" */
        if( strcmp(in_arr[j], "help") == 0 || strcmp(in_arr[j], "pwd") == 0 ){

            pid_t bpid = Fork();
            if( bpid == 0 ){

                if( strcmp(in_arr[j], "help") == 0 && j == 0 ){
                    HELP_ME();    
                    exit(0);
                }
                if( strcmp(in_arr[j], "pwd") == 0){
                    if( getcwd( (void*) cwd_buf, cwd_len) != NULL ){
                        printf("%s\n", cwd_buf);
                        exit(0);
                    }
                }
                exit(1);
            }
            wait(NULL); // wait for child to die.
            break;
        }
        /** Requested exit */
        else if( strcmp(in_arr[j], "exit") == 0 ){
            return 1;
        }
        /** Directory changing */
        else if( strcmp(in_arr[j], "cd") == 0 ){
            int k = j+1;
            if( in_arr[k] != 0){  // If cd has following input

                /* "cd -" will go to last directory we were in */
                if( strcmp(in_arr[k], "-") == 0){

                    char *old_pwd = getenv("OLDPWD");
                    if( old_pwd == NULL ){
                        printf(BUILTIN_ERROR, "OLDPWD not set\n");
                    } else {
                        save_cwd();         // will overwrite OLDPWD
                        chdir(old_pwd);
                        break;
                    }
                }
                /* "cd .." will go up one directory */
                else if( strcmp(in_arr[k], ".." ) == 0 ){
                    if( save_cwd() == 0 ){ 
                        chdir("../"); 
                        break;
                    }
                }
                /* "cd ." will stay in current directory */
                else if( strcmp(in_arr[k], "." ) == 0 ){
                    save_cwd();
                    break;
                }
                else {  /* cd is followed by a path */
                    char temp_buf[cwd_len];
                    getcwd( (void*) temp_buf, cwd_len); // save current, but not overwrite OLDPWD
                    if( chdir(in_arr[k]) < 0 ){
                        printf(BUILTIN_ERROR, "No such file or directory");
                        break;
                    }
                    /* only new OLDPWD if path exists */
                    setenv("OLDPWD", temp_buf, 1);
                    break;
                }
            } else { /* cd had no following input. So cd with no args */
                char *home_var = getenv("HOME");
                save_cwd();
                chdir(home_var);
                break;
            }
        } // end of the cd builtins

        // --------- Below are builtins for the job control --------- //
        /* For job control, block all signals.                         */

        /** the jobs argument. Print suspended jobs **/
        else if( strcmp(in_arr[j], "jobs") == 0 ){
            print_suspended_jobs();
            break;
        }
        /** fg %JID **/
        else if( strcmp(in_arr[j], "fg") == 0 ){
            Sigprocmask(SIG_BLOCK, &mask, &prev);

            if( in_arr[j+1] != NULL &&  *in_arr[j+1] == '%' ){  
                if( (continue_job(in_arr[j+1])) < 0 ){
                    printf(BUILTIN_ERROR, "no such job");
                }
            } else {
                printf(BUILTIN_ERROR, "arguments must be job IDs");
            }
            Sigprocmask(SIG_SETMASK, &prev, NULL);
            break;
        }
        /** kill PID or %JID **/
        else if( strcmp(in_arr[j], "kill") == 0 ){

            /* If there is nothing to do */
            if( in_arr[j+1] == NULL ){
                printf(BUILTIN_ERROR, "arguments must be process or job IDs");
                break;
            }

            /* Block signals and do process management */
            Sigprocmask(SIG_BLOCK, &mask, &prev);

            if(  in_arr[j+1] != NULL && *in_arr[j+1] != 0 ){

                pid_t pid = kill_job(in_arr[j+1]); // attempt to kill it
                if( pid <= 0 ){
                    printf(BUILTIN_ERROR, "no such job");
                    Sigprocmask(SIG_SETMASK, &prev, NULL);
                    break;
                }

                if( pid > 0 && kill( pid, SIGKILL) < 0 ){
                    printf(BUILTIN_ERROR, "no such job");
                } // bash doesn't print anything so we don't either

            } else { // else if not enough args
                printf(BUILTIN_ERROR, "arguments must be process or job IDs");
            }

            Sigprocmask(SIG_SETMASK, &prev, NULL);
            break;
        }
        /* The input was not a built in */
        else {
            return -1;
        }

    } // end for_loop. I don't think I needed a for loop
    return 0;
}

