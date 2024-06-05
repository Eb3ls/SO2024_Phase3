#include "sst.h"
#include "utils_phase3.h"
#include "sst_utils.h"

extern state_t uproc_state[9];
extern pcb_t* uproc_pcb[9];
extern support_t uproc_support[9];

void sst_entry() {
    support_t* support_struct = getSupportStruct();
    int asid = support_struct->sup_asid;
    STST(&uproc_state[asid]);

    uproc_state[asid].pc_epc = UPROCSTARTADDR;
    uproc_state[asid].reg_t9 = UPROCSTARTADDR;
    uproc_state[asid].reg_sp = USERSTACKTOP;
    // State will be user mode, interrupts enabled, local timer enabled
    uproc_state[asid].status = ALLOFF | USERPON | IEPON | IECON | IMON | TEBITON;
    uproc_state[asid].entry_hi = (asid << ASIDSHIFT);
    uproc_state[asid].hi = (asid << ASIDSHIFT);

    create_process(&uproc_state[asid], &uproc_support[asid]);

    ssi_payload_t* payload;

    while (1) {
        pcb_t* sender = (pcb_t*)SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, (unsigned int)&payload, 0);
        switch (payload->service_code) {
        case (GET_TOD):
            getTOD_sst(sender);
            break;
        case (TERMINATE):
            terminate_sst();
            break;
        case (WRITEPRINTER):
            writePrinter_sst(sender, asid, (sst_print_t*)payload->arg);
            break;
        case (WRITETERMINAL):
            writeTerminal_sst(sender, asid, (sst_print_t*)payload->arg);
            break;
        }
    }
}
