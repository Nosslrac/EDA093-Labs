/*
 * Main source code file for lsh shell program
 *
 * You are free to add functions to this file.
 * If you want to add functions in a separate file(s)
 * you will need to modify the CMakeLists.txt to compile
 * your additional file(s).
 *
 * Add appropriate comments in your code to make it
 * easier for us while grading your assignment.
 *
 * Using assert statements in your code is a great way to catch errors early and make debugging easier.
 * Think of them as mini self-checks that ensure your program behaves as expected.
 * By setting up these guardrails, you're creating a more robust and maintainable solution.
 * So go ahead, sprinkle some asserts in your code; they're your friends in disguise!
 *
 * All the best!
 */
#include <assert.h>
#include <ctype.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The <unistd.h> header is your gateway to the OS's process management facilities.
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h> 

#include "parse.h"
#include "ProgramState.h"

static void print_cmd(Command *cmd);
static void print_pgm(Pgm *p);
void stripwhite(char *);

//Signals
static void init_signals();
static void handle_sigint();
//When shell exit -> background processes exit
static void shell_exit();

static int handle_command(Command* cmd);
void handle_child();
void do_nothing();

//Only used in shell (parent) process
volatile ProgramState pState = {0, 0};

int main(void)
{
  // Setup signal handlers
  pid_t pid = getpid();
  init_signals();
  // All subsequent child processes will inherit this pgid -> 
  // Ctrl+c should kill all processes in pgid except pid
  setpgid(pid, pid);

  pState.foregroundChild = 0;
  pState.shellPid = pid;

  for (;;)
  {
    char *line;
    line = readline("> ");

    if(line == NULL){
      printf("Detected EOF\n");
      kill(0, SIGHUP);
      break;
    }
    // Remove leading and trailing whitespace from the line
    stripwhite(line);

    // If stripped line not blank
    if (*line)
    {
      add_history(line);

      Command cmd;
      if (parse(line, &cmd) == 1)
      {
        // If a foreground process is started, it should be terminated on SIGINT
        int retCode = handle_command(&cmd);
      }
      else
      {
        printf("Parse ERROR\n");
      }
    }

    // Clear memory
    free(line);
  }
  return EXIT_SUCCESS;
}

/*
 * Initialize all signals with custom handlers
 */
void init_signals()
{
  if(signal(SIGINT, handle_sigint) == SIG_ERR)
  {
    printf("Unable to initialize SIGINT handler");
  }
  if(signal(SIGCHLD, handle_child) == SIG_ERR)
  {
    printf("Unable to initialize SIGCHLD handler");
  }
  if(signal(SIGHUP, SIG_IGN) == SIG_ERR)
  {
    printf("Couldn't ignore SIGHUP signal");
  }
}

/*
 * Handle command passed to the shell
 * Returns a foreground process pid if started,
 * if a background process is started 0 is returned
*/
int handle_command(Command* cmd)
{
  pid_t pid = fork();
  //printf("Pid after fork %d\n", pid);
  if(pid == -1)
  {
    printf("Fork failed");
    return -1;
  }
  if(pid == 0)
  {
    // Remove signal handlers for child processes -> handled by shell process
    signal(SIGINT, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    // All child processes can be gracefully terminated when the shell exits
    signal(SIGHUP, shell_exit);
    int retCode = execvp(cmd->pgm->pgmlist[0], cmd->pgm->pgmlist);
    exit(retCode);
  }

  // Process running in foreground -> should wait for it to finish
  if(cmd->background == 0)
  {
    pState.foregroundChild = pid;
    waitpid(pid, NULL, 0);
  }
  return 0;
}

void handle_child()
{
  //Kills background processes when they finish
  pid_t child = 0;
  int status = 0;
  while((child = waitpid(0, &status, WNOHANG)) > 0 && child != pState.foregroundChild){
    assert(child != pState.foregroundChild);
    // Should we do something with exit status of child process?
    kill(child, SIGKILL);
  }
}


/* 
 * Ctrl+c should send a SIGINT signal to the foreground processes
 * and kill foreground child processes.
 */
static void handle_sigint()
{
  if(pState.foregroundChild > 0){
    pid_t deadChild = waitpid(pState.foregroundChild, NULL, WNOHANG);
    if(deadChild == -1){
      return;
    }
    assert(deadChild == 0 || deadChild == pState.foregroundChild);
    kill(pState.foregroundChild, SIGKILL);
    pState.foregroundChild = 0;
  }
}

static void shell_exit()
{
  _exit(0); // Not sure if som status should be used here??
}

/*
 * Print a Command structure as returned by parse on stdout.
 *
 * Helper function, no need to change. Might be useful to study as inspiration.
 */
static void print_cmd(Command *cmd_list)
{
  printf("------------------------------\n");
  printf("Parse OK\n");
  printf("stdin:      %s\n", cmd_list->rstdin ? cmd_list->rstdin : "<none>");
  printf("stdout:     %s\n", cmd_list->rstdout ? cmd_list->rstdout : "<none>");
  printf("background: %s\n", cmd_list->background ? "true" : "false");
  printf("Pgms:\n");
  print_pgm(cmd_list->pgm);
  printf("------------------------------\n");
}

/* Print a (linked) list of Pgm:s.
 *
 * Helper function, no need to change. Might be useful to study as inpsiration.
 */
static void print_pgm(Pgm *p)
{
  if (p == NULL)
  {
    return;
  }
  else
  {
    char **pl = p->pgmlist;

    /* The list is in reversed order so print
     * it reversed to get right
     */
    print_pgm(p->next);
    printf("            * [ ");
    while (*pl)
    {
      printf("%s ", *pl++);
    }
    printf("]\n");
  }
}


/* Strip whitespace from the start and end of a string.
 *
 * Helper function, no need to change.
 */
void stripwhite(char *string)
{
  size_t i = 0;

  while (isspace(string[i]))
  {
    i++;
  }

  if (i)
  {
    memmove(string, string + i, strlen(string + i) + 1);
  }

  i = strlen(string) - 1;
  while (i > 0 && isspace(string[i]))
  {
    i--;
  }

  string[++i] = '\0';
}
