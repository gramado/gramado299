
// ts.c
// Task switching.
// Actually it is thread scwitching.

#include <kernel.h>    

//#define TS_DEBUG

//--------------------------------------


static void __task_switch (void);
static void __on_finished_executing( struct thread_d *t );

static void cry(unsigned long flags);

//============

// ## PREEMPT ##
// Preempt
// >> MOVEMENT 3 (Running --> Ready).
// sofrendo preempção por tempo.
// Fim do quantum.
// Nesse momento a thread [esgotou] seu quantum, 
// então sofrerá preempção e outra thread será colocada 
// para rodar de acordo com a ordem estabelecida pelo escalonador.
// #todo: Mas isso só poderia acontecer se a flag
// ->preempted permitisse. 
// talvez o certo seja ->preenptable.

static void __on_finished_executing( struct thread_d *t )
{
    if ( (void*) t == NULL )
        panic("__on_finished_executing: t\n");

    if (t->magic!=1234)
        panic("__on_finished_executing: t magic\n");

// #bugbug
//  Isso está acontecendo.

    //if ( CurrentThread->state != RUNNING )
    //    panic("task_switch: CurrentThread->state != RUNNING");

    //if ( CurrentThread->state != RUNNING )
    //     goto try_next;

//
// Preempt
//
 
    if ( t->state == RUNNING )
    {
        t->state = READY;
        t->readyCount = 0;
    }

//
// Spawn thread 
//

// Check for a thread in standby.
// In this case, this routine will not return.
// See: schedi.c

    check_for_standby();   

// ---------------------------------------------------------

//
// Signals
//

// Peding signals for this thread.
// #todo: Put signals this way = t->signal |= 1<<(signal-1);

    // Se existe algum sinal para essa thread
    if (t->signal != 0)
    {
        //#test
        //if ( t->signal & (1<<(SIGALRM-1)) )
        //    printf("SIGALRM\n");
        //if ( t->signal & (1<<(SIGKILL-1)) )
        //    printf("SIGKILL\n");

        //refresh_screen();    
        //panic("__on_finished_executing: t->signal\n");
    }

// ---------------------------------------------------------

//
// Services:
// 

// Nesse momento uma thread esgotou seu quantum,
// podemos checar se tem ela alguma mensagem para o kernel,
// responter a mensagem realizando uma chamada à algum
// serviço em ring 3 ou chamando alguma rotina interna. 

// #todo
// Podemos atualizar contadores, considerando
// essa condição de termos encerrado nossos creditos.

// Essa pode ser uma boa hora pra checar o working set de uma thread.
// Quais foram as páginas mais usadas?
// Quantas páginas?

// Esse pode ser um bom momento para checar estatísticas dessa thread.
// E avaliarmos o mal-comportamento dela. Como uso de systemcalls,
// recursos, pagefaults, etc...

// Esse pode ser um bom momento para enfileirarmos essa thread
// caso o tipo de scheduler nos permita. Se for round robin,
// não precisa, mas se for outra política, então podemos
// colocar ele na fila de prontos em tail, 
// pois serão retirados em head.

    // ready_q[tail] = (unsigned long) t;




//
// == EXTRA ==========
//

// Call extra routines scheduled to this moment.
// #hackhack
// Vamos validar isso, pois isso é trabalho de uma rotina
// do timer qua ainda não esta pronta.
        
    //extra = TRUE;
    extra = FALSE;

    if (extra == TRUE)
    {
        //#debug
        //debug_print (" X "); 

        tsCallExtraRoutines();

        //KiRequest();
        //request();

        extra = FALSE;
    }

// Dead thread collector
// Avalia se é necessário acordar a thread do dead thread collector.
// É uma thread em ring 0.
// Só chamamos se ele ja estiver inicializado e rodando.
// #bugbug
// precismos rever essa questão pois isso pode estar
// fazendo a idle thread dormir. Isso pode prejudicar
// a contagem.
// See: .c
// #bugbug
// #todo: This is a work in progress!

    if (dead_thread_collector_status == TRUE){
        check_for_dead_thread_collector();
    }
    
    if(t->tid == INIT_TID)
    {
        if (t->_its_my_party_and_ill_cry_if_i_want_to == TRUE)
            cry(0);
    }
}

// :(
static void cry(unsigned long flags)
{
    if (flags & 0x8000)
        gramado_shutdown(0);
}



/*
 * task_switch:
 *     Switch the thread.
 *     Save and restore context.
 *     Select the next thread and dispatch.
 *     return to _irq0.
 *     Called by KiTaskSwitch.
 */
static void __task_switch (void)
{

// Current
    struct process_d  *CurrentProcess;
    struct thread_d   *CurrentThread;

// Target
    struct process_d  *TargetProcess;
    struct thread_d   *TargetThread;


// The owner of the current thread.
    pid_t owner_pid = (pid_t) (-1);  //fail

// tmp tid
    //tid_t tmp_tid = -1;


// =======================================================

//
// Current thread
//

// Check current thread limits.

// index

    if ( current_thread < 0 || 
         current_thread >= THREAD_COUNT_MAX )
    {
        panic ("ts: current_thread\n");
    }

// structure

    CurrentThread = (void *) threadList[current_thread]; 

    if ( (void *) CurrentThread == NULL )
    {
        panic ("ts: CurrentThread\n");
    }

    if ( CurrentThread->used != TRUE ||  
         CurrentThread->magic != 1234 )
    {
        panic ("ts: CurrentThread validation\n");
    }

// =======================================================

//
// Current process
//

// The owner of the current thread.


// pid

    owner_pid = (pid_t) CurrentThread->owner_pid;

    if ( owner_pid < 0 ||
         owner_pid >= PROCESS_COUNT_MAX )
    {
        panic ("ts: owner_pid\n");
    }

// structure

    CurrentProcess = (void *) processList[owner_pid];

    if ( (void *) CurrentProcess == NULL )
    {
        panic ("ts: CurrentProcess\n");
    }

    if ( CurrentProcess->used != TRUE ||  
         CurrentProcess->magic != 1234 )
    {
        panic ("ts: CurrentProcess validation\n");
    }

// check pid
    if ( CurrentProcess->pid != owner_pid ){
        panic("ts: CurrentProcess->pid != owner_pid \n");
    }


//
// Update the global variable.
//

    //current_process = (pid_t) owner_pid;
    set_current_process( owner_pid );

//
//  == Conting =================================
//

// 1 second = 1000 milliseconds
// sys_time_hz = 600 ticks per second.
// 1/600 de segundo a cada tick
// 1000/100 = 10 ms quando em 100HZ.
// 1000/600 = 1.x ms quando em 600HZ.
// x = 0 + (x ms); 


// step: 
// Quantas vezes ela já rodou no total.
    CurrentThread->step++; 

// runningCount: 
// Quanto tempo ela está rodando antes de parar.
    CurrentThread->runningCount++;

//
// == #bugbug =========================
//

// #bugbug
// Rever essa contagem

/*
The variables i have are:
Current->step = How many timer the thread already ran.
sys_time_hz = The timer frequency. (600Hz).
No double type, no float type.
----------
600Hz means that we have 600 ticks per second.
With 100 Hz we have 10 milliseconds per tick. ((1000/100)=10)
With 600Hz we have 1.66666666667 milliseconds per tick.   ((1000/600)=1)
------
Maybe i will try 500Hz.
With 600Hz we have 2 milliseconds per tick.   ((1000/500)=2)
----
This is a very poor incrementation method:
Current->total_time_ms = Current->total_time_ms + (1000/sys_time_hz);
The remainder ??
----
*/

// Quanto tempo em ms ele rodou no total.
    CurrentThread->total_time_ms = 
        (unsigned long) CurrentThread->total_time_ms + (DEFAULT_PIT_FREQ/sys_time_hz);

// Incrementa a quantidade de ms que ela está rodando antes de parar.
// isso precisa ser zerado quando ela reiniciar no próximo round.
    CurrentThread->runningCount_ms = 
        (unsigned long) CurrentThread->runningCount_ms + (DEFAULT_PIT_FREQ/sys_time_hz);


//
// == Locked ? ===============================
//

// Taskswitch locked? 
// Return without saving.

    if ( task_switch_status == LOCKED )
    {
        IncrementDispatcherCount (SELECT_CURRENT_COUNT);
        debug_print ("ts: Locked $\n");
        return; 
    }


// Unlocked?
// Nesse momento a thread atual sofre preempção por tempo
// Em seguida tentamos selecionar outra thread.
// Save the context.
// Not unlocked?

    if ( task_switch_status != UNLOCKED ){
        panic ("ts: task_switch_status != UNLOCKED\n");
    }

//
// Save context
//

// #todo:
// Put the tid as an argument.

    save_current_context();
    CurrentThread->saved = TRUE;

// #test
// signal? timer ?
// O contexto da thread está salvo.
// Podemos checar se ha um timer configurado 
// para esse dado tick e saltarmos para o handler
// de single shot configurado para esse timer.

    //if( (jiffies % 16) == 0 )
    //{
        //spawn_test_signal();
    //}

//=======================================================

//
// == Checar se esgotou o tempo de processamento ==
//

// #obs:
// Ja salvamos os contexto.
// Se a thread ainda não esgotou seu quantum, 
// então ela continua usando o processador.
// A preempção acontecerá por dois possíveis motivos.
// + Se o timeslice acabar.
// + Se a flag de yield foi acionada.

// Ainda não esgotou o timeslice.
// Yield support.
// preempção por pedido de yield.
// Atendendo o pedido para tirar a thread do estado de rodando
// e colocar ela no estado de pronta.
// Coloca no estado de pronto e limpa a flag.
// Em seguida vamos procurar outra.
// A mesma thread vai rodar novamente,
// Não precisa restaurar o contexto.
// O seu contexto está salvo, mas o handler em assembly
// vai usar o contexto que ele já possui.

    // Ainda não esgotou o tempo de processamento.
    // Vamos retornar e permitir que ela continue rodando,
    // ou sinalizar que
    if ( CurrentThread->runningCount < CurrentThread->quantum ){

        // Yield in progress. 
        // Esgota o quantum e ela saírá naturalmente
        // no próximo tick.
        // Revertemos a flag acionada em schedi.c.

        if ( CurrentThread->state == RUNNING && 
             CurrentThread->yield_in_progress == TRUE )
        {
            CurrentThread->runningCount = CurrentThread->quantum;
            CurrentThread->yield_in_progress = FALSE;
        }

        IncrementDispatcherCount (SELECT_CURRENT_COUNT);
        //debug_print (" The same again $\n");
        //debug_print ("s");  // the same again
        return; 

    // End of quantum:
    // Preempt: >> MOVEMENT 3 (Running --> Ready).
    // Agora esgotou o tempo de processamento.
    // Calling local worker.
    } else if ( CurrentThread->runningCount >= CurrentThread->quantum ){

        __on_finished_executing(CurrentThread);
        goto try_next;
    
    // Estamos perdidos com o tempo de processamento.
    // Can we balance it?
    }else{
        panic ("ts: CurrentThread->runningCount\n");
    };


// Crazy Fail!
// #bugbug
// Não deveríamos estar aqui.
// Podemos abortar ou selecionar a próxima provisóriamente.

    //#debug
    //panic ("ts.c: crazy fail");

    goto dispatch_current; 

//
// == TRY NEXT THREAD =====
//

try_next: 

// No threads
// #todo: Can we reintialize the kernel?
// See: up.h and cpu.h

    // No thread. 
    if (UPProcessorBlock.threads_counter == 0){
        panic("ts: No threads\n");
    }

// Only '1' thread.
// Is that thread the idle thread?
// Can we use the mwait instruction ?
// See: up.h and cpu.h
// tid0_thread
// This is a ring0 thread.
// See: x86_64/x64init.c
// If we will run only the idle thread, 
// so we can use the mwait instruction. 
// asm ("mwait"); 

    // Only 1 thread.
    // The Idle thread is gonna be the scheduler condutor.
    if (UPProcessorBlock.threads_counter == 1)
    {
        Conductor = (void *) UPProcessorBlock.IdleThread;
        goto go_ahead;
    }

// Reescalonar se chegamos ao fim do round.
// #bugbug
// Ao fim do round estamos tendo problemas ao reescalonar 
// Podemos tentar repedir o round só para teste...
// depois melhoramos o reescalonamento.
// #importante:
// #todo: #test: 
// De tempos em tempos uma interrupção pode chamar o escalonador,
// ao invés de chamarmos o escalonador ao fim de todo round.
// #critério:
// Se alcançamos o fim da lista encadeada cujo ponteiro é 'Conductor'.
// Então chamamos o scheduler para reescalonar as threads.
// Essa rotina reescalona e entrega um novo current_thread e
// Conductor.
// #?
// Estamos reconstruindo o round muitas vezes por segundo.
// Isso é ruin quando tem poucas threads, mas não faz diferença
// se o round for composto por muitas threads.

    // End of round. Rebuild the round.
    if ( (void *) Conductor->next == NULL )
    {
        current_thread = (tid_t) KiScheduler();
        goto go_ahead;
    }


// Circular.
// #critério
// Se ainda temos threads na lista encadeada, então selecionaremos
// a próxima da lista.
// #BUGBUG: ISSO PODE SER UM >>> ELSE <<< DO IF ACIMA.

    // Get the next thread in the linked list.
    if ( (void *) Conductor->next != NULL )
    {
        Conductor = (void *) Conductor->next;
        goto go_ahead;
    }

// #fail
// No thread was selected.
// Can we use the idle? or reschedule?

    //Conductor = ____IDLE;
    //goto go_ahead;

// #bugbug
// Not reached yet.
    panic ("ts: Unspected error\n");

// Go ahead
// #importante:
// Nesse momento já temos uma thread selecionada,
// vamos checar a validade e executar ela.
// #importante:
// Caso a thread selecionada não seja válida, temos duas opções,
// ou chamamos o escalonador, ou saltamos para o início dessa rotina
// para tentarmos outros critérios.

go_ahead:

// :)
//#########################################//
//  # We have a new selected thread now #  //
//#########################################//

// TARGET:
// Esse foi o ponteiro configurado pelo scheduler
// ou quando pegamos a próxima na lista.

    TargetThread = (void *) Conductor;

    if ( (void *) TargetThread == NULL )
    { 
        debug_print ("ts: Struct ");
        current_thread = (tid_t) KiScheduler();
        goto try_next;
    }

    if ( TargetThread->used != TRUE || 
         TargetThread->magic != 1234 )
    {
        debug_print ("ts: val ");
        current_thread = (tid_t) KiScheduler();
        goto try_next;
    }

    if ( TargetThread->state != READY )
    {
        debug_print ("ts: state ");
        current_thread = (tid_t) KiScheduler();
        goto try_next;
    }

//
// == Dispatcher ====
//
    
// Current selected.

    current_thread = (int) TargetThread->tid;
    goto dispatch_current;

// #debug
// Not reached
    //panic("ts: [FAIL] dispatching target\n");

// =================
// Dispatch current 
// =================

// Validation
// Check current thread limits.
// The target thread will be the current.

dispatch_current:

// tid
    if ( current_thread < 0 || 
         current_thread >= THREAD_COUNT_MAX )
    {
        panic ("ts-dispatch_current: current_thread\n");
    }

// structure
    TargetThread = (void *) threadList[current_thread];

    if ( (void *) TargetThread == NULL ){
        panic ("ts-dispatch_current: TargetThread\n");
    }
    if ( TargetThread->used  != TRUE || 
         TargetThread->magic != 1234 || 
         TargetThread->state != READY )
    {
        panic ("ts-dispatch_current: validation\n");
    }

// #todo
    //UPProcessorBlock.CurrentThread = (struct thread_d *) TargetThread;
    //UPProcessorBlock.NextThread    = (struct thread_d *) TargetThread->next;
    
// Counters
// Clean
// The spawn routine will do something more.

    TargetThread->standbyCount = 0;
    TargetThread->standbyCount_ms = 0;
    TargetThread->runningCount = 0;
    TargetThread->runningCount_ms = 0;
    TargetThread->readyCount = 0;
    TargetThread->readyCount_ms = 0;
    TargetThread->waitingCount = 0;
    TargetThread->waitingCount_ms = 0;
    TargetThread->blockedCount = 0;
    TargetThread->blockedCount_ms = 0;

// Base Priority:
    if ( TargetThread->base_priority > PRIORITY_MAX ){
        TargetThread->base_priority = PRIORITY_MAX;
    }

// Priority
    if ( TargetThread->priority > PRIORITY_MAX ){
        TargetThread->priority = PRIORITY_MAX;
    }

// Credits limit.
    if ( TargetThread->quantum > QUANTUM_MAX)
    {
        TargetThread->quantum = QUANTUM_MAX;
    }

// Call dispatcher.
// #bugbug
// Talvez aqui devemos indicar que a current foi selecionada. 
    IncrementDispatcherCount (SELECT_DISPATCHER_COUNT);

// MOVEMENT 4 (Ready --> Running).
    dispatcher(DISPATCHER_CURRENT); 

//done:
// We will return in the end of this function.

// Owner validation.
// Owner PID.

    pid_t targetthread_OwnerPID = (pid_t) TargetThread->owner_pid;

    if ( targetthread_OwnerPID < 0 || 
         targetthread_OwnerPID >= THREAD_COUNT_MAX )
    {
       printf ("ts: targetthread_OwnerPID ERROR \n", targetthread_OwnerPID );
       die();
    }

// Target process 

    TargetProcess = (void *) processList[ targetthread_OwnerPID ];

    if ( (void *) TargetProcess == NULL ){
        printf ("ts: TargetProcess %s struct fail \n", TargetProcess->name );
        die();
    }

    if ( TargetProcess->used != TRUE || 
         TargetProcess->magic != 1234 )
    {
        printf ("ts: TargetProcess %s validation \n", 
            TargetProcess->name );
        die();
    }

    if( TargetProcess->pid != targetthread_OwnerPID )
    {
        panic("ts: TargetProcess->pid != targetthread_OwnerPID\n");
    }

// Update global variable.

    //current_process = (pid_t) TargetProcess->pid;
    set_current_process (TargetProcess->pid);


// check pml4_PA

    if ( (unsigned long) TargetProcess->pml4_PA == 0 )
    {
        printf ("ts: Process %s pml4 fail\n", TargetProcess->name );
        die();
    }


// #bugug
// #todo

    // current_process_pagedirectory_address = (unsigned long) P->DirectoryPA;
    // ?? = (unsigned long) P->pml4_PA;

//#ifdef TS_DEBUG
//    debug_print ("ts: done $\n");
//#endif 

    return;

fail:
    panic ("ts: Unspected error\n");
}


/*
 * psTaskSwitch:
 *     Interface para chamar a rotina de Task Switch.
 *     KiTaskSwitch em ts.c gerencia a rotina de 
 * troca de thread, realizando operações de salvamento e 
 * restauração de contexto utilizado variáveis globais e 
 * extrutura de dados, seleciona a próxima thread através 
 * do scheduler, despacha a thread selecionada através do 
 * dispatcher e retorna para a função _irq0 em hw.inc, 
 * que configurará os registradores e executará a 
 * thread através do método iret.
 * #importante:
 * Na verdade, é uma interface pra uma rotina que 
 * faz tudo isso.
 */
 
/*
// @todo: Fazer alguma rotina antes aqui ?!
// Obs: A qui poderemos criar rotinas que não lidem com a troca de 
// threads propriamente, mas com atualizações de variáveis e gerenciamento 
// de contagem.
// >> Na entrada da rotina podemos atualizar a contagem da tarefa que acabou de rodar.
// >> A rotina task_switch fica responsável apenas troca de contexto, não fazendo 
// atualização de variáveis de contagem.
// >> ?? Na saída ??
// ?? quem atualizou as variáveis de critério de escolha ??? o dispacher ??
*/

// Called by:
// irq0_TIMER in pit.c.
// _irq0 in hw.asm. (old way?)

void psTaskSwitch(void)
{

// Filters.

// #::
// A interrupçao de timer pode acontecer 
// enquanto um processo em ring3 ou ring0 esta rodando.
// Durante essa rotina de handler de timer
// pode ser que chamaremos um callout dentro do
// window server, mas esse callout não sera interrompido
// por outra interrupçao de timer, pois ainda
// não chamamos EOI.

    pid_t current_process = (pid_t) get_current_process();


    if ( current_process < 0 || 
         current_process >= PROCESS_COUNT_MAX )
    {
        printf ("psTaskSwitch: current_process %d", current_process );
        die();
    }

    if ( current_thread < 0 || 
         current_thread >= THREAD_COUNT_MAX )
    {
        printf ("psTaskSwitch: current_thread %d", current_thread ); 
        die();
    }

// Permitindo que o assembly chame o callback.
// Somente quando o processo interrompido for o init.

    pid_t ws_pid=-1;
    ws_pid = (pid_t) socket_get_gramado_port(GRAMADO_WS_PORT);

    // Se estamos na thread do window server.
    if (current_process == ws_pid)
    {

        // Se o callback ja foi inicializado
        // por uma chamada do window server.
        if ( ws_callback_info.ready == TRUE )
        {
            //see: callback.c
            prepare_next_ws_callback();

            //no taskswitching
            return;
        }
    }

// The task switching routine.

    __task_switch();
}


/*
 * get_task_status:
 *     Obtem o status do mecanismo de taskswitch.
 * @todo: Mudar o nome dessa função para taskswitchGetStatus();.
 */

//#bugbug: Mudar para int.

unsigned long get_task_status (void)
{
    return (unsigned long) task_switch_status;
}

/*
 * set_task_status:
 *    Configura o status do mecanismo de task switch.
 *    Se o mecanismo de taskswitch estiver desligado 
 * não ocorrerá a mudança.
 * @todo: Mudar o nome dessa função para taskswitchSetStatus(.);
 */ 

// #bugbug: Mudar para int.

void set_task_status( unsigned long status )
{
    task_switch_status = (unsigned long) status;
}


void taskswitch_lock (void){
    task_switch_status = (unsigned long) LOCKED;
}

void taskswitch_unlock (void){
    task_switch_status = (unsigned long) UNLOCKED;
}


// Call extra routines scheduled to this moment.
// called by task_switch.
// #importante:
// Checaremos por atividades extras que foram agendadas pelo 
// mecanismo de request. Isso depois do contexto ter sido 
// salvo e antes de selecionarmos a próxima thread.

void tsCallExtraRoutines(void)
{

    debug_print ("tsCallExtraRoutines: [FIXME] \n");

    // Kernel requests.
    //KiRequest();

    // Unix signals.
    //KiSignal();

    // ...

    // #todo: 
    // Talvez possamos incluir mais atividades extras.
}























