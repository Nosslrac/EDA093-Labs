#ifndef LSH_INC
#define LSH_INC
//Libs
#include <stdio.h>

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
int fork_and_pipe(Pgm* pgm, size_t forks, int background);
int handle_cd(Command* cmd);
size_t get_numberOfCommands(Command* cmd);
char* get_path(char* cwd);
#endif