#include "sst_utils.h"
#include "utils_phase3.h"

void getTOD_sst(pcb_t* sender) {
    cpu_t cpu_time;
    STCK(cpu_time);
    SYSCALL(SENDMESSAGE, (unsigned int)sender, (unsigned int)(cpu_time), 0);
}

void terminate_sst() {
    ssi_payload_t term_process_payload = {
        .service_code = TERMPROCESS,
        .arg = NULL,
    };
    SYSCALL(SENDMESSAGE, (unsigned int)test_pcb, 0, 0);
    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&term_process_payload), 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, 0, 0);
}

void writePrinter_sst(pcb_t* sender, unsigned int asid, sst_print_t* payload) {
    char* msg = payload->string;
    doIOPrinter(asid, PRINTCHR, msg, payload->length);
    SYSCALL(SENDMESSAGE, (unsigned int)sender, 0, 0);
}

void writeTerminal_sst(pcb_t* sender, unsigned int asid, sst_print_t* payload) {
    char* msg = payload->string;
    doIOTerminal(asid, TRANSMITCHAR, msg, payload->length);
    SYSCALL(SENDMESSAGE, (unsigned int)sender, 0, 0);
}
