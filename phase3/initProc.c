#include "vmSupport.h"
#include "sst.h"

state_t swap_mutex_state, sst1_state, sst2_state, sst3_state, sst4_state, sst5_state, sst6_state, sst7_state, sst8_state;

pcb_t *swap_mutex_pcb, *sst1_pcb, *sst2_pcb, *sst3_pcb, *sst4_pcb, *sst5_pcb, *sst6_pcb, *sst7_pcb, *sst8_pcb;

pcb_t* create_process(state_t *s)
{
    pcb_t *p;
    ssi_create_process_t ssi_create_process = {
        .state = s,
        .support = NULL,
    };
    ssi_payload_t payload = {
        .service_code = CREATEPROCESS,
        .arg = &ssi_create_process,
    };
    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)&payload, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&p), 0);
    return p;
}

void test()
{
    unsigned int swap_pool_address_base = 0x20000000 + (32 * PAGESIZE);
    initSwapStruct();

    // IMPORTANTE:
    // ATTUALMENTE I PROCESSI FIGLI HANNO PERMESSO KERNEL
    // I VERI PERMESSI PROBABILMENTE SONO: ALLOFF | USERPON | IEPON | IMON

    // Inizializzazione del processo Swap Mutex
    STST(&swap_mutex_state);
    swap_mutex_state.reg_sp -= (2 * PAGESIZE);  // Non sappiamo bene quanto lasciare di spazio per lo stack pointer tra un processo e l'altro
    swap_mutex_state.pc_epc = (memaddr)swapMutex_entry_point;
    swap_mutex_state.status = ALLOFF | IEPON | IMON;
    
    swap_mutex_pcb = create_process(&swap_mutex_state);

    // Inizializzazione dei processi SST
    STST(&sst1_state);
    sst1_state.reg_sp = swap_mutex_state.reg_sp - (2 * PAGESIZE);
    sst1_state.pc_epc = (memaddr)sst_entry_point;
    sst1_state.status = ALLOFF | IEPON | IMON;

    sst1_pcb = create_process(&sst1_state);

    STST(&sst2_state);
    sst2_state.reg_sp = sst1_state.reg_sp - (2 * PAGESIZE);
    sst2_state.pc_epc = (memaddr)sst_entry_point;
    sst2_state.status = ALLOFF | IEPON | IMON;

    sst2_pcb = create_process(&sst2_state);

    STST(&sst3_state);
    sst3_state.reg_sp = sst2_state.reg_sp - (2 * PAGESIZE);
    sst3_state.pc_epc = (memaddr)sst_entry_point;
    sst3_state.status = ALLOFF | IEPON | IMON;

    sst3_pcb = create_process(&sst3_state);

    STST(&sst4_state);
    sst4_state.reg_sp = sst3_state.reg_sp - (2 * PAGESIZE);
    sst4_state.pc_epc = (memaddr)sst_entry_point;
    sst4_state.status = ALLOFF | IEPON | IMON;

    sst4_pcb = create_process(&sst4_state);
    
    STST(&sst5_state);
    sst5_state.reg_sp = sst4_state.reg_sp - (2 * PAGESIZE);
    sst5_state.pc_epc = (memaddr)sst_entry_point;
    sst5_state.status = ALLOFF | IEPON | IMON;

    sst5_pcb = create_process(&sst5_state);

    STST(&sst6_state);
    sst6_state.reg_sp = sst5_state.reg_sp - (2 * PAGESIZE);
    sst6_state.pc_epc = (memaddr)sst_entry_point;
    sst6_state.status = ALLOFF | IEPON | IMON;

    sst6_pcb = create_process(&sst6_state);

    STST(&sst7_state);
    sst7_state.reg_sp = sst6_state.reg_sp - (2 * PAGESIZE);
    sst7_state.pc_epc = (memaddr)sst_entry_point;
    sst7_state.status = ALLOFF | IEPON | IMON;

    sst7_pcb = create_process(&sst7_state);

    STST(&sst8_state);
    sst8_state.reg_sp = sst7_state.reg_sp - (2 * PAGESIZE);
    sst8_state.pc_epc = (memaddr)sst_entry_point;
    sst8_state.status = ALLOFF | IEPON | IMON;

    sst8_pcb = create_process(&sst8_state);

    SYSCALL(RECEIVEMESSAGE, (unsigned int)swap_mutex_pcb, 0, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)sst1_pcb, 0, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)sst2_pcb, 0, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)sst3_pcb, 0, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)sst4_pcb, 0, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)sst5_pcb, 0, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)sst6_pcb, 0, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)sst7_pcb, 0, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)sst8_pcb, 0, 0);

    HALT();
}
