#include "utils_phase3.h"

pcb_t* create_process(state_t* s, support_t* supp) {
    pcb_t* p;
    ssi_create_process_t ssi_create_process = {
        .state = s,
        .support = supp,
    };
    ssi_payload_t payload = {
        .service_code = CREATEPROCESS,
        .arg = &ssi_create_process,
    };
    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)&payload, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&p), 0);
    return p;
}

support_t* getSupportStruct() {
    support_t* support_struct;
    ssi_payload_t getsup_payload = {
        .service_code = GETSUPPORTPTR,
        .arg = NULL,
    };
    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&getsup_payload), 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&support_struct), 0);
    return support_struct;
}

unsigned int doIOSupportLevel(unsigned int command_address, unsigned int command_value) {
    unsigned int status;
    ssi_do_io_t do_io = {
        .commandAddr = (memaddr*)command_address,
        .commandValue = command_value,
    };
    ssi_payload_t payload = {
        .service_code = DOIO,
        .arg = &do_io,
    };
    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&payload), 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&status), 0);
    return status;
}

unsigned int doIOFlash(unsigned int asid, unsigned int vpn, unsigned int command, unsigned int output) {
    // Calculate the flash address
    unsigned int device_address = START_DEVREG + ((4 - 3) * 0x80) + ((asid - 1) * 0x10);
    unsigned int command_address = device_address + 0x4;
    unsigned int data0_address = device_address + 0x8;
    unsigned int command_value = (vpn << 8) | command;
    *(((unsigned int*)data0_address)) = output;
    return doIOSupportLevel(command_address, command_value);
}

void doIOPrinter(unsigned int asid, unsigned int command, char* msg, int length) {
    // Calculate the printer address
    unsigned int device_address = START_DEVREG + ((6 - 3) * 0x80) + ((asid - 1) * 0x10);
    unsigned int command_address = device_address + 0x4;
    unsigned int data0_address = device_address + 0x8;
    unsigned int command_value = command;
    for (int i = 0; i < length; i++) {
        *(((unsigned int*)data0_address)) = msg[i];
        doIOSupportLevel(command_address, command_value);
    }
}

void doIOTerminal(unsigned int asid, unsigned int command, char* msg, int length) {
    // Calculate the terminal address
    unsigned int device_address = START_DEVREG + ((7 - 3) * 0x80) + ((asid - 1) * 0x10);
    unsigned int command_address;
    if (command == TRANSMITCHAR) {
        command_address = device_address + 0xc;
    } else {
        command_address = device_address + 0x4;
    }
    for (int i = 0; i < length; i++) {
        unsigned int command_value = (((unsigned int)msg[i]) << 8) | command;
        doIOSupportLevel(command_address, command_value);
    }
}
