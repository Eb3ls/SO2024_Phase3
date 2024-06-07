#include "sst.h"
#include "sysSupport.h"
#include "utils_phase3.h"
#include "vmSupport.h"

state_t swap_mutex_state;
pcb_t* swap_mutex_pcb;

state_t uproc_state[9];
support_t uproc_support[9];

pcb_t* sst_pcb[9];

unsigned int stackTLB_array[9][500];
unsigned int stackGen_array[9][500];

void test() {
    initSwapStruct();

    // Inizializzazione del processo Swap Mutex
    STST(&swap_mutex_state);
    swap_mutex_state.reg_sp -= (2 * PAGESIZE); // Non sappiamo bene quanto lasciare di spazio per lo stack pointer tra un processo e l'altro
    swap_mutex_state.pc_epc = (memaddr)swapMutex_entry_point;
    swap_mutex_state.status = ALLOFF | IEPON | IMON;

    swap_mutex_pcb = create_process(&swap_mutex_state, NULL);

    // Inizializzazione delle strutture di supporto
    for (int i = 1; i < 9; i++) {
        support_t* support_struct = &uproc_support[i];
        support_struct->sup_asid = i;

        support_struct->sup_exceptContext[PGFAULTEXCEPT].stackPtr = (memaddr)&stackTLB_array[i][499];
        support_struct->sup_exceptContext[PGFAULTEXCEPT].status = ALLOFF | IEPON | IECON | IMON | TEBITON;
        support_struct->sup_exceptContext[PGFAULTEXCEPT].pc = (memaddr)pageFaultHandler;

        support_struct->sup_exceptContext[GENERALEXCEPT].stackPtr = (memaddr)&stackGen_array[i][499];
        support_struct->sup_exceptContext[GENERALEXCEPT].status = ALLOFF | IEPON | IECON | IMON | TEBITON;
        support_struct->sup_exceptContext[GENERALEXCEPT].pc = (memaddr)generalExceptionHandler;

        for (int j = 0; j < 31; j++) {
            support_struct->sup_privatePgTbl[j].pte_entryHI = ((0x80000 + j) << VPNSHIFT) | (i << ASIDSHIFT);
            support_struct->sup_privatePgTbl[j].pte_entryLO = DIRTYON;
        }
        support_struct->sup_privatePgTbl[31].pte_entryHI = (0xBFFFF << VPNSHIFT) | (i << ASIDSHIFT);
        support_struct->sup_privatePgTbl[31].pte_entryLO = DIRTYON;
    }

    // Inizializzazione dei processi SST

    unsigned int stackPointer = swap_mutex_state.reg_sp;

    for (int i = 1; i < 9; i++) {
        state_t* state = &uproc_state[i];
        support_t* support = &uproc_support[i];

        STST(state);
        state->reg_sp = stackPointer - (2 * PAGESIZE);
        stackPointer = state->reg_sp;
        state->pc_epc = (memaddr)sst_entry;
        state->status = ALLOFF | IEPON | IECON | IMON | TEBITON;
        state->entry_hi = i << ASIDSHIFT;

        sst_pcb[i] = create_process(state, support);
    }

    int counter = 0;
    while (counter < 8) {
        SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, 0, 0);
        counter++;
    }

    ssi_payload_t term_process_payload = {
        .service_code = TERMPROCESS,
        .arg = NULL,
    };
    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&term_process_payload), 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, 0, 0);
}
