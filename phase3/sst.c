#include "sst.h"
#include "utils_phase3.h"

extern support_t support_structs[9];

void get_TOD_sst(ssi_payload_t* payload, pcb_t* sender){
    cpu_t cpu_time;
    STCK(cpu_time);
    SYSCALL(SENDMESSAGE, (unsigned int)sender, (unsigned int)(cpu_time), 0);
}

void terminate_process_sst(ssi_payload_t* payload, pcb_t* sender){
    ssi_payload_t term_process_payload = {
        .service_code = TERMPROCESS,
        .arg = NULL,
    };
    // SYSCALL AL TEST PER NOTIFICARE LA MORTE
    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&term_process_payload), 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, 0, 0);
}

void write_terminal_sst(ssi_payload_t* payload, pcb_t* sender, support_t* support_structure){
    // Get ASID
    unsigned int asid = support_structure->sup_asid;
    // Get the print structure
    sst_print_t* print = (sst_print_t*) payload->arg;
    // Get the length of the message
    unsigned int length = print->length;
    // Get the message
    char* msg = print->string;

    unsigned int terminal_address = START_DEVREG + ((7 - 3) * 0x80) + ((asid - 1) * 0x10);
    unsigned int command = terminal_address + 0xc;
    unsigned int status;

    for (unsigned int i = 0; i < length; i++){
        unsigned int value = PRINTCHR | (((unsigned int)*msg) << 8);
        ssi_do_io_t do_io = {
            .commandAddr = (memaddr*)command,
            .commandValue = value,
        };
        ssi_payload_t payload = {
            .service_code = DOIO,
            .arg = &do_io,
        };
        SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&payload), 0);
        SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&status), 0);
        msg++;
    }
    // Send the message to the sender
    SYSCALL(SENDMESSAGE, (unsigned int)sender, 0, 0);
}

void sst_entry_point(){
    SYSCALL(SENDMESSAGE, (unsigned int)test_pcb, 0, 0);
    support_t* support_structure = getSupportStruct();
    state_t uproc_state;
    pcb_t* uproc_pcb;
    if (support_structure->sup_asid == 1){
        STST(&uproc_state);
        unsigned int uproc_number = 1;

        uproc_state.pc_epc = UPROCSTARTADDR;
        uproc_state.reg_t9 = UPROCSTARTADDR;
        uproc_state.reg_sp = USERSTACKTOP;
        // State will be user mode, interrupts enabled, local timer enabled
        uproc_state.status = ALLOFF | USERPON | IEPON | IMON | TEBITON;
        uproc_state.entry_hi = (uproc_number << ASIDSHIFT);

        uproc_pcb = create_process(&uproc_state, &support_structs[uproc_number]);
    }
    while(1){
        ssi_payload_t* payload;
        pcb_t* sender = (pcb_t*) SYSCALL(RECEIVEMESSAGE, ANYMESSAGE, (unsigned int) &payload, 0);
        switch(payload->service_code){
            case (GET_TOD):
                get_TOD_sst(payload, sender);
                break;
            case (TERMINATE):
                terminate_process_sst(payload, sender);
                break;
            case (WRITEPRINTER):
                // Poi ci pensiamo
                break;
            case (WRITETERMINAL):
                write_terminal_sst(payload, sender, support_structure);
                break;
        }
    }
}
