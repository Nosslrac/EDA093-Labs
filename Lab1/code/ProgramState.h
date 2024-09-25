#include <time.h>

#define MAX_PIPELEN 16
/*
typedef struct{
    pid_t pid;
    int terminated;
} ChildProcess;
*/

typedef struct{
    pid_t shellPid;
    pid_t foregroundChild;
} ProgramState;
/*
void add_child(ProgramState* pState, pid_t cpid);
void terminate_child(ProgramState* pState, pid_t cpid);
*/