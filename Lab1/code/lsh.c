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
#include <sys/stat.h>
#include <fcntl.h>


#include "lsh.h"
#include "parse.h"
#include "ProgramState.h"

//static void print_cmd(Command *cmd);
//static void print_pgm(Pgm *p);


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
  char cwd[1024];

  for (;;)
  {
    char *line;
    char* start = get_path(cwd);
    line = readline(start);

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
        exit_handler(&cmd);
        //print_cmd(&cmd);
        // If a foreground process is started, it should be terminated on SIGINT
        if(handle_cd(&cmd) == 0)
        {
          //print_cmd(&cmd);
          int retCode = handle_command(&cmd);
          if(retCode == -1){
            printf("Command failed\n");
          }
        }
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
    printf("Unable to initialize SIGINT handler\n");
  }
  if(signal(SIGCHLD, handle_child) == SIG_ERR)
  {
    printf("Unable to initialize SIGCHLD handler\n");
  }
  if(signal(SIGHUP, SIG_IGN) == SIG_ERR)
  {
    printf("Couldn't ignore SIGHUP signal\n");
  }
}

char* get_path(char* cwd)
{
  char* end = cwd;
  getcwd(cwd, 1024 * sizeof(char));
  while(*end++ != 0);
  end--;
  *end++ = '>';
  *end++ = ' ';
  *end++ = 0;
  int numSlashes = 0;
  while(numSlashes < 2 && end > cwd)
  {
    if(*end-- == '/')
    {
      numSlashes++;
    }
  }
  *end = '$';
  //printf(end);
  return end;
}

/*
 * If cd command -> execute cd and return 1
 * else return 0
 */
int handle_cd(Command* cmd)
{
  if(strcmp(cmd->pgm->pgmlist[0], "cd") == 0)
  {
    int retVal = chdir(cmd->pgm->pgmlist[1]);
    if(retVal == -1)
    {
      printf("cd: No such file or directory\n");
    }
    return 1;
  }
  return 0;
}



/*
 * Handle command passed to the shell
 * Returns a foreground process pid if started,
 * if a background process is started 0 is returned
*/
int execute_command(Pgm* pgm)
{
  // Remove signal handlers for child processes -> handled by shell process
  signal(SIGINT, SIG_IGN);
  signal(SIGCHLD, SIG_IGN);
  // All child processes can be gracefully terminated when the shell exits
  signal(SIGHUP, child_exit);
  int retCode = execvp(pgm->pgmlist[0], pgm->pgmlist);
  exit(retCode);
}



int handle_command(Command* cmd)
{
  // Terminal process fork
  pid_t process = fork();

  if(process == 0) 
  {
    //Non shell process
    if(cmd->rstdout)
    {
      //Redirect output to file
      int fileOut = open(cmd->rstdout, O_RDWR | O_CREAT, S_IRWXU);
      dup2(fileOut, STDOUT_FILENO);
      close(fileOut);
    }
    if(cmd->rstdin)
    {
      // Input file content to command
      int fileIn = open(cmd->rstdin, O_RDONLY, S_IRWXU);
      dup2(fileIn, STDIN_FILENO);
      close(fileIn);
    }
    size_t numCommands = get_numberOfCommands(cmd);
    forkAndPipe(cmd->pgm, numCommands - 1, NULL);
  }
  else if(cmd->background == 0) 
  {
    //Shell process wait for forked child processes if it's not a background task
    pState.foregroundChild = process;
    waitpid(process, NULL, 0);
  }
  return 0;
}

int forkAndPipe(Pgm* pgm, size_t forks, int* prevpipe)
{
  if(prevpipe != NULL)
  {
    //Child process should redirect stdout to pipe write end
    close(prevpipe[0]); //Close read end
    dup2(prevpipe[1], STDOUT_FILENO); //Redir stdout to write end of pipe
    close(prevpipe[1]); // Close old fd
  }
  if(forks == 0){
    //Recursion base case
    return execute_command(pgm);
  }
 
  //Create the pipe
  int pipefd[2];
  int success = pipe(pipefd);
  if(success == -1)
  {
    printf("Couldn't create pipe\n");
    return -1;
  }

  pid_t child = fork();
  if(child == -1){
    printf("Failed fork\n");
    return -1;
  }

  if(child == 0)
  {
    //Child process will continue forking
    return forkAndPipe(pgm->next, forks-1, pipefd);
  }
  //Parent process - should redirect sdtin to read end of pipe.
  close(pipefd[1]);
  dup2(pipefd[0], STDIN_FILENO);
  close(pipefd[0]);
  return execute_command(pgm);
}


int get_numberOfCommands(Command* cmd)
{
  int numCommands = 1;
  struct c* next = cmd->pgm->next;
  while(next != NULL){
    numCommands++;
    next = next->next;
  }
  return numCommands;
}


/*
 * If a child process changes its state it's handled here
 */
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

void exit_handler(Command* cmd)
{
  if(strcmp(cmd->pgm->pgmlist[0], "exit") == 0)
  {
    killpg(0, SIGHUP);
    exit(0);
  }
}

/* 
 * Ctrl+c should send a SIGINT signal to the foreground processes
 * and kill foreground child processes.
 */
void handle_sigint()
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

/*
 * Gracefully terminate child processes when shell exits
 */
void child_exit()
{
  _exit(0); // Not sure if som status should be used here??
}

/*
 * Print a Command structure as returned by parse on stdout.
 *
 * Helper function, no need to change. Might be useful to study as inspiration.
 */
 /*
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
 */

/* Print a (linked) list of Pgm:s.
 *
 * Helper function, no need to change. Might be useful to study as inpsiration.
 */
 /*
static void print_pgm(Pgm *p)
{
  if (p == NULL)
  {
    return;
  }
  else
  {
    char **pl = p->pgmlist;

    // The list is in reversed order so print
     // it reversed to get right
     
    print_pgm(p->next);
    printf("            * [ ");
    while (*pl)
    {
      printf("%s ", *pl++);
    }
    printf("]\n");
  }
}
 */


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
