Fase 2

Rispetto alla prima parte del progetto abbiamo modificato la struttura pcb_t inserendo un attributo (p_location) che si occupa di immagazzinare in quale lista si trova il processo. Questo ci ha permesso di organizzare meglio la struttura delle SYSCALL e della gestione dei processi in generale in tutto il codice.

Scheduler:
Abbiamo deciso di tenere la struttura dello scheduler semplice e concisa. Anche grazie ad un'assunzione riguardo al controllo di HALT del sistema.
Nello specifico abbiamo assunto che se non è più presente alcun processo all'interno della readyQueue e ne è presente solo uno attivo nel sistema, esso sia l'SSI.
Di conseguenza ci siamo limitati a semplici controlli sui contatori dei processi.

SYSCALL:
Abbiamo implementato le SYSCALL sfruttando il campo aggiuntivo alla struttura pcb_t discussa sopra.
Nello specifico sfruttiamo il campo per mettere un processo in attesa di una RECEIVE, senza aggiungerlo ad alcuna coda.
Quindi, al momento di una SEND che possa sbloccare il processo in attesa, viene controllato il campo p_location.
Se esso risulta in attesa di una SYSCALL, allora è necessario "svegliarlo" inserendolo nella readyQueue.
Alternativamente si inserisce semplicemente il messaggio nel suo inbox.

SSI:
Abbiamo implementato l'SSI come un processo server che segue le stesse politiche di tutti gli altri processi (in termini di scheduling).
Per quanto riguarda il servizio di doIO, è richiesto salvare nella posizione corretta i processi in attesa di una operazione su device.
Perciò è stato necessario ricercare l'index corrispondente al device da contattare all'interno dell'array dei blockedPCBs.
Eseguiamo la ricerca di questo index tramite la funzione findDevice.
Questa funzione, dato l'indirizzo del device, prima ricerca la linea corrispondente e successivamente calcola il numero del device.
Tramite questi due dati è possibile trovare l'index ricercato.

Gestione delle eccezioni:
Al sollevarsi di un'eccezione viene eseguita la funzione exceptionHandler, la quale si occupa di lanciare le varie funzioni dedite a gestire i vari casi possibili.

Interrupts:
Nel caso di un'eccezione Interrupt, questa viene gestita tramite l'interruptHandler, che a sua volta si occupa di avviare la funzione corretta disambiguando la causa dell'interrupt dal cause register.

Non-Timer Interrupts:
Per la gestione di questo tipo di interrupt, è stato necessario ottenere l'indirizzo del device che ha sollevato l'eccezione.
Per trovare questo indirizzo ci serviamo (come nell'SSI) dei valori di linea e del numero del device.
A questo punto, operando con la struttura corretta (a seconda se l'interrupt è stato lanciato dal terminale o da un generico device) possiamo scrivere l'ACK sull'indirizzo di comando del device e possiamo ottenere il risultato dell'operazione richiesta mediante l'indirizzo di stato.
Procediamo ad inserire direttamente nell'inbox del processo che ha richiesto l'operazione un messaggio contenente lo stato ottenuto, e a reinserire il processo (che a questo punto vede la propria richista soddisfatta) nella readyQueue.
NOTA: per indirizzare correttamente un processo in blockedPcbs[] dati linea e numero del device, si è deciso di immaginare l'array come suddiviso in blocchi da 8 elementi, ovvero considerando il numero massimo di device che possono essere collegati a una linea. Di conseguenza, gli indici da 0 a 7 si riferiscono alle liste dei device della prima linea, da 8 a 15 della seconda e via dicendo, fino ad utilizzare 40 dei 49 indici disponibili. Determinare un indirizzamento univoco per i processi in attesa è cruciale al fine di rendere SSI e interrupt handler compatibili nel loro operato.

PLT Interrupts:
Come da specifiche, il gestore degli interrupt del Local Timer garantisce che i processi si distribuiscano equamente le risorse di calcolo del processore, facendo tornare il processo corrente in readyQueue nel momento che viene lanciando l'interrupt.

PseudoClock Interrupts:
Questo tipo di interrupt risveglia tutti i processi in attesa della Tick, rimuovendoli uno per volta dalla rispettiva coda e aggiungendoli alla readyQueue. Inoltre, per risolvere la receive "pendente" di tali processi, viene pushato direttamente un messaggio nella loro inbox come se fosse l'SSI a farlo, di modo da poter chiudere immediatamente la richiesta senza dover passare nuovamente la gestione del servizio all'SSI.

PassUpOrDie:
Abbiamo implementato il PassUpOrDie come un semplice controllo sulla struttura di supporto del processo.
In caso non sia presente alcuna struttura di supporto procediamo con la terminazione del processo che ha causato l'eccezione.
Altrimenti procediamo a copiare lo stato dalla BIOSDATAPAGE allo stato della struttura di supporto ed eseguiamo LDCTX.