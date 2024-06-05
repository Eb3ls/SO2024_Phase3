#include "vmSupport.h"
#include "sst.h"
#include "sysSupport.h"
#include "utils_phase3.h"

state_t swap_mutex_state, sst1_state, sst2_state, sst3_state, sst4_state, sst5_state, sst6_state, sst7_state, sst8_state, uproc1_state;

pcb_t *swap_mutex_pcb, *sst1_pcb, *sst2_pcb, *sst3_pcb, *sst4_pcb, *sst5_pcb, *sst6_pcb, *sst7_pcb, *sst8_pcb, *uproc1_pcb;

support_t support_structs[9]; // 0 is not used, 1-8 are used for user support structures, to match ASID

unsigned int swap_pool_address_base = RAMSTART + (32 * PAGESIZE);

void initialize_support_struct(support_t* support, unsigned int asid){
    support->sup_asid = asid;

    // Commentato perchè gli stati iniziali sono già settati a 0
    // support->sup_exceptState[0] = 0; // STATO PRIMA DEL PAGE FAULT
    // support->sup_exceptState[1] = 0; // STATO PRIMA DEL GENERAL EXCEPTION

    unsigned int stackTLB_array[500];
    unsigned int stackGen_array[500];

    // CONTESTO PER LA GESTIONE DEL PAGE FAULT
    support->sup_exceptContext[0].stackPtr = (memaddr) &(stackTLB_array[499]);
    support->sup_exceptContext[0].status = ALLOFF | IEPON | IMON | TEBITON;
    support->sup_exceptContext[0].pc = (memaddr)pageFaultHandler;

    // COSTESTO PER LA GESTIONE DEL GENERAL EXCEPTION
    support->sup_exceptContext[1].stackPtr = (memaddr) &(stackGen_array[499]);
    support->sup_exceptContext[1].status = ALLOFF | IEPON | IMON | TEBITON;
    support->sup_exceptContext[1].pc = (memaddr)generalExceptionHandler;

    // INIZIALIZZAZIONE DELLA PAGE TABLE PRIVATA
    for(int i = 0; i < 31; i++){
        support->sup_privatePgTbl[i].pte_entryHI = ((0x80000 + i) << VPNSHIFT) | (asid << ASIDSHIFT);
        support->sup_privatePgTbl[i].pte_entryLO = DIRTYON;
    }
    support->sup_privatePgTbl[31].pte_entryHI = (0xBFFFF << VPNSHIFT) | (asid << ASIDSHIFT);
    support->sup_privatePgTbl[31].pte_entryLO = DIRTYON;
}

void test()
{
    initSwapStruct();

    // Inizializzazione delle strutture di supporto
    for(int i = 1; i < 9; i++){
        initialize_support_struct(&support_structs[i], i);
    }

    // IMPORTANTE:
    // ATTUALMENTE I PROCESSI FIGLI HANNO PERMESSO KERNEL
    // I VERI PERMESSI PROBABILMENTE SONO: ALLOFF | USERPON | IEPON | IMON
    // Forse anche TEBITON

    // Inizializzazione del processo Swap Mutex
    STST(&swap_mutex_state);
    swap_mutex_state.reg_sp -= (2 * PAGESIZE);  // Non sappiamo bene quanto lasciare di spazio per lo stack pointer tra un processo e l'altro
    swap_mutex_state.pc_epc = (memaddr)swapMutex_entry_point;
    swap_mutex_state.status = ALLOFF | IEPON | IMON;

    swap_mutex_pcb = create_process(&swap_mutex_state, NULL);

    // Inizializzazione dei processi SST
    STST(&sst1_state);
    sst1_state.reg_sp = swap_mutex_state.reg_sp - (2 * PAGESIZE);
    sst1_state.pc_epc = (memaddr)sst_entry_point;
    sst1_state.status = ALLOFF | IEPON | IMON;
    sst1_state.entry_hi = 1 << ASIDSHIFT;

    sst1_pcb = create_process(&sst1_state, &support_structs[1]);

    STST(&sst2_state);
    sst2_state.reg_sp = sst1_state.reg_sp - (2 * PAGESIZE);
    sst2_state.pc_epc = (memaddr)sst_entry_point;
    sst2_state.status = ALLOFF | IEPON | IMON;
    sst2_state.entry_hi = 2 << ASIDSHIFT;

    sst2_pcb = create_process(&sst2_state, &support_structs[2]);

    STST(&sst3_state);
    sst3_state.reg_sp = sst2_state.reg_sp - (2 * PAGESIZE);
    sst3_state.pc_epc = (memaddr)sst_entry_point;
    sst3_state.status = ALLOFF | IEPON | IMON;
    sst3_state.entry_hi = 3 << ASIDSHIFT;

    sst3_pcb = create_process(&sst3_state, &support_structs[3]);

    STST(&sst4_state);
    sst4_state.reg_sp = sst3_state.reg_sp - (2 * PAGESIZE);
    sst4_state.pc_epc = (memaddr)sst_entry_point;
    sst4_state.status = ALLOFF | IEPON | IMON;
    sst4_state.entry_hi = 4 << ASIDSHIFT;

    sst4_pcb = create_process(&sst4_state, &support_structs[4]);

    STST(&sst5_state);
    sst5_state.reg_sp = sst4_state.reg_sp - (2 * PAGESIZE);
    sst5_state.pc_epc = (memaddr)sst_entry_point;
    sst5_state.status = ALLOFF | IEPON | IMON;
    sst5_state.entry_hi = 5 << ASIDSHIFT;

    sst5_pcb = create_process(&sst5_state, &support_structs[5]);

    STST(&sst6_state);
    sst6_state.reg_sp = sst5_state.reg_sp - (2 * PAGESIZE);
    sst6_state.pc_epc = (memaddr)sst_entry_point;
    sst6_state.status = ALLOFF | IEPON | IMON;
    sst6_state.entry_hi = 6 << ASIDSHIFT;

    sst6_pcb = create_process(&sst6_state, &support_structs[6]);

    STST(&sst7_state);
    sst7_state.reg_sp = sst6_state.reg_sp - (2 * PAGESIZE);
    sst7_state.pc_epc = (memaddr)sst_entry_point;
    sst7_state.status = ALLOFF | IEPON | IMON;
    sst7_state.entry_hi = 7 << ASIDSHIFT;

    sst7_pcb = create_process(&sst7_state, &support_structs[7]);

    STST(&sst8_state);
    sst8_state.reg_sp = sst7_state.reg_sp - (2 * PAGESIZE);
    sst8_state.pc_epc = (memaddr)sst_entry_point;
    sst8_state.status = ALLOFF | IEPON | IMON;
    sst8_state.entry_hi = 8 << ASIDSHIFT;

    sst8_pcb = create_process(&sst8_state, &support_structs[8]);

    SYSCALL(RECEIVEMESSAGE, (unsigned int)swap_mutex_pcb, 0, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)sst1_pcb, 0, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)sst2_pcb, 0, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)sst3_pcb, 0, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)sst4_pcb, 0, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)sst5_pcb, 0, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)sst6_pcb, 0, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)sst7_pcb, 0, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)sst8_pcb, 0, 0);

    SYSCALL(RECEIVEMESSAGE, (unsigned int)test_pcb, 0, 0);

    HALT();
}
