#include "sst.h"

void get_TOD_sst(ssi_payload_t* payload, pcb_t* sender){
    cpu_t cpu_time;
    STCK(cpu_time);
    SYSCALL(SENDMESSAGE, (unsigned int)sender, (unsigned int)(&cpu_time), 0);
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

void sst_entry_point(){
    SYSCALL(SENDMESSAGE, (unsigned int)test_pcb, 0, 0);
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
                // Poi ci pensiamo
                break;
        }
    }
}
