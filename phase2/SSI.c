#include "SSI.h"

extern struct list_head pcbFree_h;

int findDevice(unsigned int devAddr){
    // This is used to convert the device address to the index on the blockedPCBs array
    // in the doIO service
    unsigned int devLine = devAddr - 0x10000054;
    unsigned int devNo = devLine;
    unsigned int line;

    if(devLine >= LOCALTIMERINT && devLine < TIMERINTERRUPT){
        line = 1;
    }
    else if(devLine >= TIMERINTERRUPT && devLine < DISKINTERRUPT){
        line = 2;
    }
    else if(devLine >= DISKINTERRUPT && devLine < FLASHINTERRUPT){
        line = 3;
    }
    else if(devLine >= FLASHINTERRUPT && devLine < PRINTINTERRUPT){
        line = 4;
        // In this part, the FLASHINTERRUPT and NETWORKINTERRUPT lines are combined
        // because NETWORKINTERRUPT does not exist
    }
    else if(devLine >= PRINTINTERRUPT && devLine < TERMINTERRUPT){
        line = 6;
    }
    else if(devLine >= TERMINTERRUPT){
        line = 7;
    }
    // Remove the line from the device address
    devNo = (devNo - ((line - 3) * 0x80)) / 0x10;
    return 8 * (line - 3) + devNo;
}



void SSI_function_entry_point(){
    while(1){
        ssi_payload_t* payload;
        pcb_t* sender = (pcb_t*) SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, (unsigned int) &payload, 0);
        int service = payload->service_code;
        void* arg = payload->arg;
        switch(service){
            case (CREATEPROCESS):
                // Create process funtion
                createProcess(sender, arg);
                break;
            case (TERMPROCESS):
                // Terminate process function
                terminateProcess(sender, arg);
                break;
            case (DOIO):
                // DoIO function
                doIO(sender, arg);
                break;
            case (GETTIME):
                // Get time function
                getTime(sender);
                break;
            case (CLOCKWAIT): 
                // Clock wait function
                waitForClock(sender);
                break;
            case (GETSUPPORTPTR):
                // Get support pointer function
                getSupportData(sender);
                break;
            case (GETPROCESSID):
                // Get process id function
                getProcessId(sender, arg);
                break;
            default:
                // If the requested service is not valid, terminate the process
                terminateProcess(sender, NULL);
                break;
        }
    }   
}

void createProcess(struct pcb_t* sender, struct ssi_create_process_t* arg){
    if(process_count >= MAXPROC){
        return;
    }
    pcb_t* new_process = allocPcb();
    copyState(&new_process->p_s, arg->state);
    new_process->p_supportStruct = arg->support;
    insertChild(sender, new_process);
    insertInList(new_process, READYQUEUE_LOCATION);
    process_count++;
    SYSCALL(SENDMESSAGE, (unsigned int) sender, (unsigned int) new_process, 0);
}

void terminateProcess(struct pcb_t* sender, struct pcb_t* arg){
    pcb_t* to_delete = NULL;
    if(arg == NULL){
        to_delete = sender;
    }
    else{
        to_delete = arg;
    }

    // Remove the process from the parent's child list
    outChild(to_delete);

    // For each child of to_delete
    while(!emptyChild(to_delete)){
        pcb_t* child = container_of(to_delete->p_child.next, pcb_t, p_sib);
        terminateProcess(child, NULL);
    }

    removeFromList(to_delete, to_delete->p_location);

    pcb_t* pcbInFree = outProcQ(&pcbFree_h, to_delete);
    if (pcbInFree == NULL){
        // If the process is not in the free list, free it
        process_count--;
        freePcb(to_delete);
    }

    if (arg != NULL){
        SYSCALL(SENDMESSAGE, (unsigned int) sender, 0, 0);
    }
}

void doIO(struct pcb_t* sender, struct ssi_do_io_t* doio){
    softBlockCount++;

    unsigned int devAddr = ((unsigned int) doio->commandAddr - 0xc);
    int index = findDevice(devAddr);

    removeFromList(sender, sender->p_location);
    insertInList(sender, index);

    *doio->commandAddr = doio->commandValue;
}

void getTime(struct pcb_t* sender){
    unsigned int time = sender->p_time;
    SYSCALL(SENDMESSAGE, (unsigned int) sender, time, 0);
}

void waitForClock(struct pcb_t* sender){
    softBlockCount++;
    removeFromList(sender, sender->p_location);
    insertInList(sender, WAITINGCLOCK_LOCATION);
}

void getSupportData(struct pcb_t* sender){
    SYSCALL(SENDMESSAGE, (unsigned int) sender, (unsigned int) sender->p_supportStruct, 0);
}

void getProcessId(struct pcb_t* sender, void* arg){
    pcb_t* process = NULL;
    if(arg == NULL){
        process = sender;
    }
    else{
        process = sender->p_parent;
    }
    SYSCALL(SENDMESSAGE, (unsigned int) sender, process->p_pid, 0);
}
