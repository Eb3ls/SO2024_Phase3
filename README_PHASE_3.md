# Relazione Fase 3

# *SST*

La funzione principale dell'implementazione dell'SST è sst_entry(). Essa si occupa di:
- ottenere la struttura di supporto del processo, anche al fine di conoscerne l'asid, mediante una funzione che richiede tale servizio all'SSI
- settare correttamente lo state del processo utente relativo all'SST che esegue la funzione e creare tale processo (si utilizza anche in questo caso una funzione che fa richiesta all'SSI)
- mettersi in ascolto di eventuali richieste effettuate tramite SYSCALL dal processo utente.
Le varie richieste, disambiguate tramite il payload del messaggio ricevuto, vengono poi risolte mediante funzioni specifiche (contenute in sst_utils.c)

In particolare:
- getTOD_sst() si occupa semplicemente di restituire al processo utente il tempo trascorso, in millisecondi, dall'ultimo avvio o riavvio del sistema, tramite la funzione STCK() fornita
- terminate_sst() comunica al processo test la fine del servizio e demanda all'SSI la terminazione dell'SST e del relativo processo utente
- writePrinter_sst() e writeTerminal_sst() gestiscono stampa su printer e terminale tramite altre funzioni (utils_phase3.c)

Queste ultime operano in maniera simile.  

Nel caso del terminale, mediante opportuni calcoli sugli indirizzi di memoria, si ottiene l'indirizzo del registro di comando del device, diverso a seconda del tipo di richiesta, che sia stata una ricezione o una trasmissione. A questo punto, in un loop, viene impostato il comando corretto per la gestione di un carattere alla volta e demandata a un'altra funzione l'invio della richiesta all'SSI, con le strutture relative opportunamente impostate.

Nel caso del printer, dopo aver ottenuto gli indirizzi del registro di comando e del registro DATA0 mediante operazioni simili, si utilizza nuovamente un loop al fine di gestire la stampa carattere per carattere. Il carattere da stampare viene scritto in DATA0 e viene chiamata la funzione sopracitata per l'invio della richiesta relativa al carattere all'SSI.

# *GESTIONE ECCEZIONI*

## Gestione page fault:

La gestione delle page fault è rappresentata dalla funzione pageFaultHandler(), la quale può gestire i 3 casi che possono generare un page fault:
1. TLB invalido su un'istruzione di caricamento
2. TLB invalido su un'istruzione di salvataggio
3. Entry del TLB non modificata (non DIRTY)

I primi due casi vengono gestiti attraverso la funzione TLBInvalidHandler(), mentre il terzo caso viene gestito come trap. Questo perchè nel nostro sistema operativo tutte le entries sono segnate come DIRTY.

### PAGER

Per la gestione di una page fault, si acquisisce inizialmente mutua esclusione. Una volta che essa è garantita, si ricavano VPN e ASID dalla struttura di supporto. Essendo utilizzato un sistema "Round Robin" per la scelta della pagina da sostituire in swap table, è sufficiente riferirsi alla variabile roundRobinPick per indicizzare la pagina da manipolare. Una volta ottenuta la pagina, possono verificarsi due casistiche:

1. La pagina è già utilizzata e il dirty bit è a 1
2. La pagina è libera

Nel primo caso, è necessario innanzitutto liberare la pagina.
Si sposta la pagina dalla swap pool al flash drive corrispondente per liberare spazio in quella entry, invalidando la pagina nella swap table e nel TLB (per mantenere coerenza) ed eseguendo un'operazione doIO al flash device relativo al processo.

Ora che ci si è ricondotti a una pagina libera, si può caricare la pagina dal flash device alla swap pool. Successivamente, è necessario aggiornare la swap table e il TLB con i dati della nuova pagina caricata e, infine, aggiornare la variabile roundRobinPick e rilasciare mutua esclusione. 

## Gestione errori generali:

La gestione delle eccezioni non-TLB è rappresentata dalla funzione generalExceptionHandler(), la quale nel momento del lancio di un'eccezione disambigua la sua natura accedendo al Cause register dalla struttura di supporto del processo corrente.

### SYSCALL

Nel caso si tratti di una SYSCALL, il controllo è demandato al relativo gestore. Dalla struttura di supporto si ottiene lo stato del processore nel momento dell'eccezione (indice 1 in quanto è una SYSCALL) per accedere ai registri. Si valuta dapprima se si tratti di una send o di una receive confrontando il registro a0 con le rispettive costanti, e successivamente si discerne se la SYSCALL coinvolge il processo parent del processo corrente (costante PARENT) o in alternativa un altro processo qualsiasi, mediante confronti con il registro a1. 
L'invio del messaggio è demandato alla funzione di libreria SYSCALL con parametri dipendenti dai confronti sopracitati e ritorno salvato in v0_code. Tale valore, che rappresenta l'esito dell'operazione viene scritto nel rispettivo registro dello stato del processore, viene aumentato il PC e viene ricaricato lo stato del processore di modo che possa continuare la sua routine da dove era stato interrotto.

### Program Trap

Nel caso invece si tratti di una program trap, questa viene gestita, ponendo attenzione alla mutua esclusione, tramite la funzione programTrapHandler().
Nello specifico i casi che possono verificarsi sono:
1. L'ingresso nella funzione proveniendo da un page fault. (Avevamo mutua esclusione)
2. L'ingresso nella funzione proveniendo direttamente da un errore del processo

A loro volta questi casi si diramano in ulteriori 2:
1. Il processo che ha scatenato l'eccezione è un processo utente
2. Il processo che ha scatenato l'eccezione è un SST

Se il processo è processo utente assumiamo che abbia causato l'eccezione direttamente dall'esecuzione del codice, oppure abbia appena rilasciato la mutua esclusione uscendo da un page fault.

Se il processo è un SST, assumiamo che il processo utente non abbia mutua esclusione poichè sta aspettando la risposta dell'SST.

Con queste assunzioni, possiamo dividere l'approccio in 2 casi:

1. Se il processo è un processo utente, allora contattiamo l'SST chiedendo di eseguire un'azione di terminate.
2. Se il processo è un processo SST, allora contattiamo l'SSI chiedendo la morte del processo stesso e di tutta la sua progenie.

# *TLB-REFILL*

Per sopperire alla mancanza di una pagina in cache, viene invocata la funzione di refill del TLB; questa funzione si occupa, nello specifico, di:

1. Ottenere il VPN della pagina richiesta 
2. Ottenere la entry relativa al VPN nella page table privata del processo corrente 
3. Caricare la entry trovata in una posizione casuale del TLB 

Si sottolinea che non è necessario controllare che la pagina sia effettivamente presente in swap pool, in quanto in tal caso il valid bit sarebbe a 0 e al momento dell'accesso verrebbe sollevato un page fault.


# *TESTER*

Il processo test si occupa di inizializzare il processo swap_mutex (che garantirà la mutua esclusione per l'accesso alla swap pool), le strutture di supporto per ogni processo utente (atte a garantire l'adeguata gestione di errori a livello supporto) e i processi SST (che offriranno ai processi utente determinati servizi).
1. Inizializzazione swap table: in questa parte vengono inizializzati i campi relativi all'asid, al vpn e al pte di ogni entry della swap table (eseguita attraverso la funzione initSwapStruct).
2. Inizializzazione swap_mutex: per la generazione del processo swap_mutex, vengono impostati i vari campi del processo.
    - Lo stack pointer a due frame di distanza da quello del processo test
    - Il program counter alla funzione di entry point dello swap_mutex
    - Lo status in kernel mode con interrupt e local timer abilitati
3. Inizializzazione delle strutture di supporto: per ogni processo utente il tester provvede ad inizializzare i campi della relativa struttura di supporto.
In particolare vengono inizializzati i campi relativi al contesto di eccezione, in base quindi a un eccezione generale o di page fault. 
Per affrontare separatamente questi due casi, vengono inizialmente generati due stack per ogni processo utente: stackTLB e stackGen. A questi punterà lo stack pointer della struttura di supporto rispettivamente nel contesto di page fault ed eccezione generale. La distinzione in base al contesto viene trattata analogamente anche per l'assegnamento del program counter che nel primo caso punterà alla funzione pageFaultHandler e nel secondo alla funzione generalExeptionHandler.
Infine per ogni struttura di supporto viene inizializzata la page table privata, inserendo per ogni entry i corretti VPN e ASID e settandone il bit di controllo a DIRTY. 
4. Inizializzazione dei processi SST: per ogni processo utente viene istanziato il corrispondente SST che condividerà con il processo figlio la struttura di supporto. 
Vengono impostati quindi:
    - Lo stack pointer, analogamente a quanto fatto per lo swap_mutex, distanziato di due frame da quest'ultimo
    - Il program counter alla funzione sst_entry
    - Lo status in kernel mode con interrupt e local timer abilitati

In seguito a queste operazioni il processo test attende un messaggio dagli 8 processi generati e successivamente richiede all'SSI di essere terminato.
