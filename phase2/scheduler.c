#include "scheduler.h"

void scheduling(){
    current_process = removeProcQ(&readyQueue);
    if (current_process != NULL){
        // Load PLT timer
        setTIMER(TIMESLICE);
        LDST(&current_process->p_s);
    }
    else{
        if (process_count == 1){
            HALT();
        }
        else if (process_count > 0 && softBlockCount > 0){
            setSTATUS(IECON | IMON);
            WAIT();
        }
        else if (process_count > 0 && softBlockCount == 0){
            PANIC();
        }
    }
}