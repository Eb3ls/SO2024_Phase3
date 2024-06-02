#include "sysSupport.h"

extern pcb_t* swap_mutex_pcb;
static int roundRobinPick = 0;
extern swap_t swapTable[2 * UPROCMAX];

support_t* getSupportStruct(){
    support_t* support_struct;
    ssi_payload_t getsup_payload = {
        .service_code = GETSUPPORTPTR,
        .arg = NULL,
    };
    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&getsup_payload), 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&support_struct), 0);
    return support_struct;
}

void TLBInvalidHandler(support_t* support_struct){
    state_t* exception_state = &(support_struct->sup_exceptState[0]);

    // ATTENZIONE: Probabilmente bisogna fare il get di questo dato dal campo entry_hi dello stato
    // all'interno della support_struct e non dal campo cause dello stato dell'eccezione
    // unsigned int cause_state = exception_state->entry_hi;
    unsigned int cause_state = exception_state->cause;

    SYSCALL(SENDMESSAGE, (unsigned int)swap_mutex_pcb, 0, 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)swap_mutex_pcb, 0, 0);

    // Get VPN and ASID from the cause register
    unsigned int vpn = ((cause_state) >> VPNSHIFT) & GETPAGENO;
    unsigned int asid = ((cause_state) >> ASIDSHIFT); // Dobbiamo isolare l'ASID!!

    // If the entry is occupied and dirty, swap it out
    if (swapTable[roundRobinPick].sw_asid != -1 && (((swapTable[roundRobinPick].sw_pte)->pte_entryLO & DIRTYON) == DIRTYON)){
        unsigned int current_processor_status = ((state_t*) BIOSDATAPAGE)->status;
        setSTATUS(ALLOFF);

        swapTable[roundRobinPick].sw_pte->pte_entryLO = swapTable[roundRobinPick].sw_pte->pte_entryLO & (!VALIDON);
        TLBCLR();

        setSTATUS(current_processor_status);

        // Bisogna scrivere la pagina in memoria secondaria
        // Bisogna capire bene come sostituire correttamente la pagina
        // Verosimilmente bisogna prendere l'indirizzo del device, scrivere/leggere in DATA0 il dato
        // da scrivere e poi inviare con l'SSI il comando per scrivere/leggere il dato`
        // devregtr status;
        // ssi_do_io_t do_io = {
        //     .commandAddr = command,
        //     .commandValue = value,
        // };
        // ssi_payload_t payload = {
        //     .service_code = DOIO,
        //     .arg = &do_io,
        // };
        // SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&payload), 0);
        // SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&status), 0);
    }
    // Se siamo arrivati qui significa che quella entry è libera, quindi possiamo 
    // caricare quello che ci pare
    // Leggiamo dal device la pagina da caricare
    // devregtr status;
    // ssi_do_io_t do_io = {
    //     .commandAddr = command,
    //     .commandValue = value,
    // };
    // ssi_payload_t payload = {
    //     .service_code = DOIO,
    //     .arg = &do_io,
    // };
    // SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&payload), 0);
    // SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&status), 0);
    
    // Quando il device ha finito di leggere la pagina, possiamo caricarla in memoria
    // e settare la entry della TLB

    unsigned int current_processor_status = ((state_t*) BIOSDATAPAGE)->status;
    setSTATUS(ALLOFF);

    swapTable[roundRobinPick].sw_asid = asid;
    swapTable[roundRobinPick].sw_pageNo = vpn;
    swapTable[roundRobinPick].sw_pte = &(support_struct->sup_privatePgTbl[vpn]);

    support_struct->sup_privatePgTbl[vpn].pte_entryHI = (vpn << VPNSHIFT) | (asid << ASIDSHIFT);
    // DOBBIAMO POPOLARE IL PFN, DOVREBBE ESSERE 12 (LEONARDO)
    support_struct->sup_privatePgTbl[vpn].pte_entryLO = VALIDON | DIRTYON | (roundRobinPick << 12);

    // Settiamo la entry della TLB
    TLBCLR();
    setENTRYHI((vpn << VPNSHIFT) | (asid << ASIDSHIFT));
    setENTRYLO(VALIDON | DIRTYON);
    TLBWR();

    setSTATUS(current_processor_status);

    roundRobinPick = (roundRobinPick + 1) % (2 * UPROCMAX);

    SYSCALL(SENDMESSAGE, (unsigned int)swap_mutex_pcb, 0, 0);
    LDST(&(support_struct->sup_exceptState[0]));
}

void pageFaultHandler(){
    support_t* support_struct = getSupportStruct();
    state_t* exception_state = &(support_struct->sup_exceptState[0]);
    switch ((exception_state->cause & GETEXECCODE) >> CAUSESHIFT){
        case TLBINVLDL:
            TLBInvalidHandler(support_struct);
            break;
        case TLBINVLDS:
            TLBInvalidHandler(support_struct);
            break;
        default:
            // "treat this case as a program trap"
            programTrapHandler();
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

void programTrapHandler(){
    // Abbiamo fatto questa ricerca del padre per killare sia il processo sia il padre (l'SST)
    // assumendo che sia la cosa giusta da fare... Forse è meglio killare solo il processo
    // ma a quel punto l'SST rimarrebbe in attesa di un messaggio che non arriverà mai

    pcb_t* parent_pcb;
    ssi_payload_t payload = {
        .service_code = GETPROCESSID,
        .arg = (void *)1,
    };
    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&payload), 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&parent_pcb), 0);
    
    ssi_payload_t term_process_payload = {
        .service_code = TERMPROCESS,
        .arg = NULL,
    };
    if (parent_pcb != test_pcb){
        term_process_payload.arg = parent_pcb;
    }
    SYSCALL(SENDMESSAGE, (unsigned int)ssi_pcb, (unsigned int)(&term_process_payload), 0);
    SYSCALL(RECEIVEMESSAGE, (unsigned int)ssi_pcb, 0, 0);
}

void generalExceptionHandler(){
    support_t* support_struct = getSupportStruct();
    state_t* exception_state = &(support_struct->sup_exceptState[1]);
    switch ((exception_state->cause & GETEXECCODE) >> CAUSESHIFT){
        case SYSEXCEPTION:
            SYSCALLExceptionHandler(current_process->p_supportStruct);
            break;
        default:
            // "treat this case as a program trap"
            programTrapHandler();
            break;
    }
}
