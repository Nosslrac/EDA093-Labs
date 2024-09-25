#ifndef LSH_INC
#define LSH_INC
//Libs
#include <stdio.h>
#include <time.h>

#include "parse.h"



void stripwhite(char *);

void init_signals();
//Signal handlers
void handle_sigint();
void child_exit();
void handle_child();
void exit_handler(Command* cmd);

int execute_command(Pgm* pgm);
int execute_command_background(Pgm* pgm);
int handle_command(Command* cmd);
int check_command(Pgm* pgm);
int handle_cd(Command* cmd);

// Fork form shell
int setup_command_chain(Command* cmd);
void childProc_setSig(Command* cmd);
void setInputOutput(Command* cmd, int index, int size);
int wait_children(const pid_t* children, int size);


size_t get_numberOfCommands(Command* cmd);
char* get_path(char* cwd);
#endif