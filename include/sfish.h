#ifndef SFISH_H
#define SFISH_H

/* Format Strings */
#define EXEC_NOT_FOUND "sfish: %s: command not found\n"
#define JOBS_LIST_ITEM "[%d] %s\n"
#define STRFTIME_RPRMT "%a %b %e, %I:%M%p"


/* From document update */
#define BUILTIN_ERROR  "sfish builtin error: %s\n"
#define SYNTAX_ERROR   "sfish syntax error: %s\n"
#define EXEC_ERROR     "sfish exec error: %s\n"




/** ------- My stuff  -------- **/

//#define HELP_MENU "National Suicide Prevention Lifeline : 1-800-273-8255\n"
#define PROMPT "\x1b[1;34m %s : hhyao >>\x1b[0m"

#define CWD_BUF 256         // for storing the cwd
#define ARGS_BUF_LEN 1024   // for the argument[]


/* Function declarations */
int is_builtins(char* input, char* in_arr[]);
int do_exe(char* in_arr[]);
int redirection( char* input, char *in_arr[]);
void io_pipe( char* in_arr[] );
int ec_pipe( char* in_arr[] );


/** -------  Extra credit things  -------- **/

// src: wikipedia. hard coded colored prompts.
#define RED     "\x1b[1;31m %s : hhyao >>\x1b[0m"
#define GRN     "\x1b[1;32m %s : hhyao >>\x1b[0m"
#define YEL     "\x1b[1;33m %s : hhyao >>\x1b[0m"
#define BLU     "\x1b[1;34m %s : hhyao >>\x1b[0m"
#define MAG     "\x1b[1;35m %s : hhyao >>\x1b[0m"
#define CYN     "\x1b[1;36m %s : hhyao >>\x1b[0m"
#define WHT     "\x1b[1;37m %s : hhyao >>\x1b[0m"
#define BWN     "\x1b[0;33m %s : hhyao >>\x1b[0m"

#define COLOR_USAGE "arguments must be COLOR:\n\t[ RED | GRN | YEL | BLU | MAG | CYN | WHT | BWN ]"

#define HELP_ME() do {                                             \
    printf(                                                         \
            ""                                                            \
            " __    __          _____ __   \n"                              \
            "(_ |_||_ |  |     |_  | (_ |_|\n"                              \
            "__)| ||__|__|__   |  _|___)| |  °º¤ø,¸¸,ø¤º°`°º¤ø,¸,ø¤°º¤ø,¸¸,¤º°¤,¸ \n" \
            "\n"                                                               \
            "CSE 320 Fall 2017: sfish - a C Shell\n"                                \
            "\n"                                                        \
            "Type 'help' to display help menu.\n"                       \
            "Type 'exit' to exit sfish.\n"                              \
            "\n"                                                        \
            "Builtins:\n"                                               \
            "             \n"                                           \
            " cd:    [ . | .. | - | PATH ] change the current working directory\n" \
            " pwd:   print name of current working directory\n"                 \
            "\n"            \
            " jobs:  print the list of currently suspended jobs\n"              \
            " fg:    [ %sJID ] move a suspended job into foreground\n"           \
            " kill:  [ %sJID | PID ] terminate the specified job\n"              \
            "\n"           \
            " color  [ COLOR ] change the prompt color. Colors include\n"           \
            "        [ RED | GRN | YEL | BLU | MAG | CYN | WHT | BWN ]\n"           \
            "\n"           \
            "\n",           \
            "%", "%");                                                      \
  } while (0)


#endif