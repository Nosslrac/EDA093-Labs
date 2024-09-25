#include "ProgramState.h"

/*
void add_child(ProgramState* pState, pid_t cpid)
{
    for(size_t i = 0; i < MAX_PIPELEN; i++){
        if(pState->foregroundChildren[i].terminated){
            pState->foregroundChildren[i].pid = cpid;
            pState->foregroundChildren[i].terminated = 0;
            break;
        }
    }
}

void terminate_child(ProgramState* pState, pid_t cpid)
{

    for(size_t i = 0; i < MAX_PIPELEN; i++){
        if(pState->foregroundChildren[i].pid == cpid){
            pState->foregroundChildren[i].terminated = 1;
            break;
        }
    }
}
*/