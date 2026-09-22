#include "sysSupport.h"
#include "sst_utils.h"
#include "utils_phase3.h"

extern pcb_t* swap_mutex_pcb;
static int roundRobinPick = 0;
extern swap_t swapTable[2 * UPROCMAX];
extern unsigned int swap_pool_address_base;

extern pcb_t* sst_pcb[9];

void TLBInvalidHandler(support_t* support_struct){
    unsigned int entryHi = support_struct->sup_exceptState[0].entry_hi;

    SYSCALL(SENDMESSAGE, (unsigned int)swap_mutex_pcb, 0, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)swap_mutex_pcb, 0, 0);

    unsigned int vpn = ((entryHi & GETPAGENO) >> VPNSHIFT);

    if (vpn > 31){
        vpn = 31;
    }

    unsigned int asid = support_struct->sup_asid;

    // Find the address of the page to be removed from memory
    unsigned int return_position = swap_pool_address_base + (roundRobinPick * PAGESIZE);

    // If the entry is occupied and dirty, swap it out
    if (swapTable[roundRobinPick].sw_asid != -1 && (((swapTable[roundRobinPick].sw_pte)->pte_entryLO & DIRTYON) == DIRTYON)){
        unsigned int current_processor_status = ((state_t*) BIOSDATAPAGE)->status;
        setSTATUS(ALLOFF);

        swapTable[roundRobinPick].sw_pte->pte_entryLO = swapTable[roundRobinPick].sw_pte->pte_entryLO & (~VALIDON);

        // Set the TLB entry
        setENTRYHI(swapTable[roundRobinPick].sw_pte->pte_entryHI);
        TLBP();
        if ((getINDEX() & PRESENTFLAG) == 0){
            setENTRYHI(swapTable[roundRobinPick].sw_pte->pte_entryHI);
            setENTRYLO(swapTable[roundRobinPick].sw_pte->pte_entryLO);
            TLBWI();
        }

        setSTATUS(current_processor_status);

        // Perform a DOIO via the SSI to initiate page write
        unsigned int exit_status = doIOFlash(swapTable[roundRobinPick].sw_asid, swapTable[roundRobinPick].sw_pageNo, FLASHWRITE, return_position);

        // If the write operation encountered an error, print an error message, release
        // mutual exclusion and terminate
        if (exit_status != 1){
            doIOTerminal(swapTable[roundRobinPick].sw_asid, PRINTCHR,
                         "Error writing to flash\n", sizeof("Error writing to flash\n") - 1);
            SYSCALL(SENDMESSAGE, (unsigned int)swap_mutex_pcb, 0, 0);
            programTrapHandler(support_struct);
        }
    }

    // If we have reached this point, it means that the entry is free, so we can
    // load the page from the flash into the swap pool

    // Perform a DOIO via the SSI to initiate page read
    unsigned int exit_status = doIOFlash(asid, vpn, FLASHREAD, return_position);

    // If the read operation encountered an error, print an error message, release
    // mutual exclusion and terminate
    if (exit_status != 1){
        doIOTerminal(asid, PRINTCHR,
                     "Error reading from flash\n", sizeof("Error reading from flash\n") - 1);
        SYSCALL(SENDMESSAGE, (unsigned int)swap_mutex_pcb, 0, 0);
        programTrapHandler(support_struct);
    }

    // If we have reached this point, it means that the read operation was successful and the page 
    // has been loaded into the swap pool

    // So we have to update the entry in the swap table with the new values (of the newly loaded page)
    // This part needs to be executed atomically without interrupts, so we save
    // the processor state, disable interrupts, update the swap table, and
    // restore the processor state with SETSTATUS()

    unsigned int current_processor_status = ((state_t*) BIOSDATAPAGE)->status;
    setSTATUS(ALLOFF);

    swapTable[roundRobinPick].sw_asid = asid;
    swapTable[roundRobinPick].sw_pageNo = vpn;
    swapTable[roundRobinPick].sw_pte = &(support_struct->sup_privatePgTbl[vpn]);

    // Now we need to update the entry of the process's private page table

    // Set the entryHI of the page
    // This line is probably redundant:
    // support_struct->sup_privatePgTbl[vpn].pte_entryHI = (entryHi >> VPNSHIFT) << VPNSHIFT | (asid << ASIDSHIFT);

    // Set the entryLO of the page
    // We have to set the VALIDON and DIRTYON bits
    // and also set the pfn of the page
    unsigned int pfn = (((unsigned int)return_position) >> 12);
    support_struct->sup_privatePgTbl[vpn].pte_entryLO = (pfn << 12) | DIRTYON | VALIDON;

    // Set the TLB entry
    setENTRYHI(support_struct->sup_privatePgTbl[vpn].pte_entryHI);
    TLBP();
    if ((getINDEX() & PRESENTFLAG) == 0){
        setENTRYHI(support_struct->sup_privatePgTbl[vpn].pte_entryHI);
        setENTRYLO(support_struct->sup_privatePgTbl[vpn].pte_entryLO);
        TLBWI();
    }

    setSTATUS(current_processor_status);

    // Increase the round robin pick for the next round
    roundRobinPick = (roundRobinPick + 1) % (2 * UPROCMAX);

    SYSCALL(SENDMESSAGE, (unsigned int)swap_mutex_pcb, 0, 0);
    LDST(&(support_struct->sup_exceptState[0]));
}

void pageFaultHandler(){
    support_t* support_struct = getSupportStruct();
    switch ((support_struct->sup_exceptState[0].cause & GETEXECCODE) >> CAUSESHIFT){
        case TLBINVLDL:
            TLBInvalidHandler(support_struct);
            break;
        case TLBINVLDS:
            TLBInvalidHandler(support_struct);
            break;
        default:
            programTrapHandler(support_struct);
            break;
    }
}

void SYSCALLExceptionHandler(support_t* support_struct){
    state_t* state = &(support_struct->sup_exceptState[1]);
    unsigned int v0_code;
    if(state->reg_a0 == SENDMSG){
        if (state->reg_a1 == PARENT){
            v0_code = SYSCALL(SENDMESSAGE, (unsigned int)current_process->p_parent, (unsigned int)state->reg_a2, 0);
        }
        else{
            v0_code = SYSCALL(SENDMESSAGE, (unsigned int)state->reg_a1, (unsigned int)state->reg_a2, 0);
        }
    }
    else if(state->reg_a0 == RECEIVEMSG){
        if (state->reg_a1 == PARENT){
            v0_code = SYSCALL(RECEIVEMESSAGE, (unsigned int)current_process->p_parent, (unsigned int)state->reg_a2, 0);
        }
        else{
            v0_code = SYSCALL(RECEIVEMESSAGE, (unsigned int)state->reg_a1, (unsigned int)state->reg_a2, 0);
        }
    }
    current_process->p_supportStruct->sup_exceptState[1].reg_v0 = v0_code;
    current_process->p_supportStruct->sup_exceptState[1].pc_epc += 4;
    LDST(&(current_process->p_supportStruct->sup_exceptState[1]));
}

void programTrapHandler(support_t* support_struct) {
    // We can access this function from two points:
    // 1. From program execution (We didn't have mutual exclusion)
    // 2. From handling a page fault (We released mutual exclusion before entering)

    // Furthermore, we can be in two situations:
    // 1. The process is a user process
    // 2. The process is an SST process

    // If we are a user process, we need to ask the SST to terminate the process
    // If we are the SST, we assume that the user process did not cause any page fault
    // after the request to the SST.

    int parent_pid;

    ssi_payload_t payload = {
        .service_code = GETPROCESSID,
        .arg = (void*)1,
    };
    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&payload), 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&parent_pid), 0);

    ssi_payload_t term_process_payload = {
        .service_code = TERMPROCESS,
        .arg = NULL,
    };
    if (parent_pid != test_pcb->p_pid) {
        // If we enter here, it means that we are a user process
        SYSCALL(SENDMESSAGE, (unsigned int)(sst_pcb[support_struct->sup_asid]), (unsigned int)(&term_process_payload), 0);
        SYSCALL(RECEIVEMESSAGE, (unsigned int)(sst_pcb[support_struct->sup_asid]), 0, 0);
    }
    else {
        // The SST must notify test before asking the SSI to tear it down.
        terminate_sst();
    }
}

void generalExceptionHandler() {
    support_t* support_struct = getSupportStruct();
    switch ((support_struct->sup_exceptState[1].cause & GETEXECCODE) >> CAUSESHIFT) {
    case SYSEXCEPTION:
        SYSCALLExceptionHandler(current_process->p_supportStruct);
        break;
    default:
        programTrapHandler(support_struct);
        break;
    }
}
