#include <time.h>

typedef struct{
    pid_t shellPid;
    pid_t foregroundChild;
} ProgramState;