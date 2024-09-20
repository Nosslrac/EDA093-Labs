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
int handle_command(Command* cmd);
int forkAndPipe(Pgm* pgm, size_t forks, int* pipe);
int handle_cd(Command* cmd);
int get_numberOfCommands(Command* cmd);
char* get_path(char* cwd);
#endif