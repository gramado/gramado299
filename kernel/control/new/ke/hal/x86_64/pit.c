/*
 * File: pit.c 
 * Descrição:
 *     Arquivo principal do driver do PIT. 
 *     Trata a irq0.
 *     Rotinas envolvendo tempo.
 *  Ambiente: 
 *     (RING 0).
 *      Módulo interno, dentro do kernenel base.
 * @todo: Criar funções que criam a estrutura timer ...
 * que será usada peloa aplicativos na forma de objeto .
 * elas devem ter um contador que enviará mensagens para o 
 * aplicativo sempre que se esgota a contagem.
 * History:
 *     2013 - Created by Fred Nora.
 */

// See:
// https://wiki.osdev.org/Programmable_Interval_Timer
// https://en.wikipedia.org/wiki/Intel_8253

/*
I/O port     Usage
0x40         Channel 0 data port (read/write)
0x41         Channel 1 data port (read/write)
0x42         Channel 2 data port (read/write)
0x43         Mode/Command register (write only, a read is ignored)

Bits         Usage
 6 and 7      Select channel :
                 0 0 = Channel 0
                 0 1 = Channel 1
                 1 0 = Channel 2
                 1 1 = Read-back command (8254 only)
 4 and 5      Access mode :
                 0 0 = Latch count value command
                 0 1 = Access mode: lobyte only
                 1 0 = Access mode: hibyte only
                 1 1 = Access mode: lobyte/hibyte
 1 to 3       Operating mode :
                 0 0 0 = Mode 0 (interrupt on terminal count)
                 0 0 1 = Mode 1 (hardware re-triggerable one-shot)
                 0 1 0 = Mode 2 (rate generator)
                 0 1 1 = Mode 3 (square wave generator)
                 1 0 0 = Mode 4 (software triggered strobe)
                 1 0 1 = Mode 5 (hardware triggered strobe)
                 1 1 0 = Mode 2 (rate generator, same as 010b)
                 1 1 1 = Mode 3 (square wave generator, same as 011b)
 0            BCD/Binary mode: 0 = 16-bit binary, 1 = four-digit BCD
*/ 

/*
 PIT info:
 ========
 PIT: mode=2 count=0x48d3 (18643) - 64.00 Hz (ch=0)
 PIT: mode=3 count=0x10000 (65536) - 18.20 Hz (ch=0) (Oracle, Virtualbox).
 PIT: mode=3 count=0x2e9a (11930) - 100.01 Hz (ch=0) (Oracle, Virtualbox).
 PIT: mode=3 count=0x2e9b (11931) - 100.00 Hz (ch=0) (Oracle, Virtualbox).
 */



#include <kernel.h>  


// total ticks
unsigned long jiffies=0;
// por quantos segundos o sistema esta rodando
// jiffies/sys_time_hz
unsigned long seconds=0;
// Por quantos ms o sistema esta rodando.
unsigned long sys_time_ms=0;


/*
struct master_timer_d
{
    int initialized;
    
    pid_t pid;
    unsigned long callback_address;
};
struct master_timer_d  MasterTimer;
*/


//Contador de ticks.
//unsigned long timerTicks;

//??  
//int timerColor;

//??
//unsigned long timerLine;

//??
//unsigned long timerIdleState;

//??
//int timerLock;

//??
//int timerError;

// #deprecated
// Text cursor
static int timerShowTextCursor = FALSE;


//??
//unsigned long timerCountSeconds;  //Count Seconds.
//...

//
// == Private functions: prototypes ===================
//

static int timerTimer(void);


// =============================

// Constructor
// Main globals.
static int timerTimer(void)
{

// total ticks
    jiffies = 0;

// por quantos segundos o sistema esta rodando
// jiffies/sys_time_hz
    seconds = 0; 

// Por quantos ms o sistema esta rodando.
    sys_time_ms = 0; 

// pit frequency
    sys_time_hz = 0;

    UpdateScreenFlag = FALSE;

// Profiler
    profiler_ticks_count = 0;
    profiler_ticks_limit = PROFILER_TICKS_DEFAULT_LIMIT;

    // ...

    return 0;
}


/*
 * irq0_TIMER:
 *     Chama o handler do kernel que está no kernel base.
 * #todo: Observar alguns procedimentos antes de chamar a rotina.
 */

// Called by:
// _irq0 in hw.asm
// It is up to the interrupt service routine to reset the latch. 
// It does that by setting bit 7 of port 0x61 (system control port B).

__VOID_IRQ 
irq0_TIMER (void)
{

// Calling the timer routine.
// See: pic.h
    DeviceInterface_PIT();

// Calling the taskswitching routine.
// See: ps/disp/ts.c
// #todo #bugbug
// nesse momento pode acontecer que teremos que
// efetuar spawn de uma thread que esta em standby,
// O nosso contexto ja estara salvo quando formos efetuar
// o spawn, mas precisamos avisar a rotina de spawn
// que precisamos realizar o EOI pois estamos num handler de pit.


// The spawn routine need to make a eoi for pit interrupt.
// Tell the spawn routine that we need a eoi.
// In the case of spawning a new thread.
    spawn_set_eoi_state();


// See: ps/disp/ts.c
    psTaskSwitch();


// The spawn routine do not need to make a eoi.
// Tell the spawn routine that we do not need a eoi anymore.
// The assembly routine will do that for us.
    spawn_reset_eoi_state();
}

extern unsigned long _callback_address_saved;


//static int __CallbackFlagInterval = (16*4);  // 15
//static int __CallbackFlagInterval = (16*3);  // 20
//static int __CallbackFlagInterval = (16*2);  // 31 :)
static int __CallbackFlagInterval = (16*1);  // 62

// #bugbug
// Quando a flag é acionada, 
// anda temos que esperar o scheduler selecionar o ws
// para vermos o callback sendo chamado.

// Worker
// Scheduler
// Ignoramos a tid retornada pela rotina.
void DeviceInterface_PIT(void)
{
    jiffies++;

//--------------------
// It is time to flush the dirty rectangles
// in the window server.
// 1000/16*4 = 15,625 fps.
// #test 60fps com o pit a 1000.
// 1000/16*1 = 62.5
// 1000/16*2 = 31,25
// 1000/16*3 = 20,83
// 1000/16*4 = 15,625
// ::: Reset callback stuff.
// Reconfigura os dados sobre o callback.
// #todo: Podemos ter uma estrutura para eles.
// Acho que o assembly importa esses valores,
// e é mais difícil importar de estruturas.
    //if ( (jiffies % (16*4)) == 0 )
    if ( (jiffies % (__CallbackFlagInterval)) == 0 )
    {
        // reinitialize callback based on the saved value.
        if ( ws_callback_info.initialized == TRUE )
        {
            if ( ws_callback_info.callback_address_saved != 0 )
                setup_callback(ws_callback_info.callback_address_saved);
        }
    }
//--------------------


/*
    if ( (jiffies % DEFAULT_PIT_FREQ) == 0 )
    {
        KiScheduler();
    }
*/
}

//-------------------------

/*
// read back
unsigned read_pit_count(void);
unsigned read_pit_count(void) 
{
	unsigned count = 0;
 
	// Disable interrupts
	cli();
 
	// al = channel in bits 6 and 7, remaining bits clear
	outb(0x43,0b0000000);
 
	count = inb(0x40);		// Low byte
	count |= inb(0x40)<<8;		// High byte
 
	return count;
}
*/

/*
void set_pit_count(unsigned count);
void set_pit_count(unsigned count) 
{
	// Disable interrupts
	cli();
 
	// Set low byte
	outb(0x40,count&0xFF);		// Low byte
	outb(0x40,(count&0xFF00)>>8);	// High byte
	return;
}
*/


/*
 * timerInit8253:
 *    @todo: Compreender melhor isso.
 * Seta a frequência de funcionamento do 
 * controlador 8253. "3579545 / 3" 
 * instead of 1193182 Hz. 
 * Reprograma o timer 8253 para operar 
 * à uma frequencia de "HZ".
 * Obs: Essa rotina substitui a rotina init_8253.
 */

//ex: 
// 0x36
// 00 | 11 | 011 | 0
// Channel 0 | word | square wave generator | 16bit binary

/*
Bits         Usage
6 and 7      Select channel :
                0 0 = Channel 0
                0 1 = Channel 1
                1 0 = Channel 2
                1 1 = Read-back command (8254 only)
4 and 5      Access mode :
                0 0 = Latch count value command
                0 1 = Access mode: lobyte only
                1 0 = Access mode: hibyte only
                1 1 = Access mode: lobyte/hibyte
1 to 3       Operating mode :
                0 0 0 = Mode 0 (interrupt on terminal count)
                0 0 1 = Mode 1 (hardware re-triggerable one-shot)
                0 1 0 = Mode 2 (rate generator)
                0 1 1 = Mode 3 (square wave generator)
                1 0 0 = Mode 4 (software triggered strobe)
                1 0 1 = Mode 5 (hardware triggered strobe)
                1 1 0 = Mode 2 (rate generator, same as 010b)
                1 1 1 = Mode 3 (square wave generator, same as 011b)
0            BCD/Binary mode: 0 = 16-bit binary, 1 = four-digit BCD
*/


// VT8237
// Compativel com intel 8254
void timerInit8253 ( unsigned int freq )
{
    unsigned int clocks_per_sec = (unsigned int) (freq & 0xFFFFFFFF);

// para 1.1 MHz
// The counter counts down to zero, then 
// sends a hardware interrupt (IRQ 0) to the CPU.
// 3579545/3

    unsigned int period = (unsigned int) ( (1193182) / clocks_per_sec );

    // #debug
    //printf("Period %d\n",period);
    //refresh_screen();
    //while(1){}


// 0x36
// 00 | 11 | 011 | 0
// Channel 0 | word | square wave generator | 16bit binary

    out8 ( 0x43, (unsigned char) 0x36 );
    io_delay();

// LSB
    out8 ( 0x40, (unsigned char) (period & 0xFF) ); 
    io_delay();

// MSB
    out8 ( 0x40, (unsigned char)(period >> 8) & 0xFF );
    io_delay();

// GLOBAL VARIABLE.
    sys_time_hz = (unsigned long) (freq & 0xFFFFFFFF);
}



// set_timeout: #todo 
void set_timeout ( unsigned long ticks )
{
    time_out = (unsigned long) ticks;
}

// get_timeout: #todo
unsigned long get_timeout (void)
{
    return (unsigned long) time_out;
}

// set_quantum:
void set_quantum ( unsigned long q)
{
    quantum = (unsigned long) q;
}

// get_quantum:
unsigned long get_quantum (void)
{
    return (unsigned long ) quantum;
}


// set_current_quantum: 
void set_current_quantum (unsigned long q)
{
    current_quantum = (unsigned long) q;
}

// get_current_quantum:
unsigned long get_current_quantum (void)
{
    return (unsigned long ) current_quantum;
}

// set_next_quantum:
void set_next_quantum (unsigned long q)
{
    next_quantum = (unsigned long) q;
}

// get_next_quantum:
unsigned long get_next_quantum (void)
{
    return (unsigned long ) next_quantum;
}  


void set_systime_hz ( unsigned long hz )
{
    sys_time_hz = hz;
}

// systime hz
unsigned long get_systime_hz (void)
{
    return (unsigned long) sys_time_hz;
}


// systime in ms
unsigned long get_systime_ms (void)
{
    return (unsigned long) sys_time_ms;
}


// get_systime_totalticks:
unsigned long get_systime_totalticks(void)
{
    return (unsigned long) jiffies;
}


/*
 * get_systime_info:
 */
// #todo 
// Criar um enum para isso.
// Comentar uma descrição em cada item.

unsigned long get_systime_info(int n)
{
    switch (n){
    case 1: return (unsigned long) get_systime_hz();          break;
    case 2: return (unsigned long) get_systime_ms();          break;
    case 3: return (unsigned long) get_systime_totalticks();  break;
    // ...
    default:  
        return (unsigned long) 0;  
        break;
    };

    // fail.
    return (unsigned long) 0;
}


int new_timer_id (void)
{
    int i=0;
    unsigned long new=0;


    for ( i=0; i<32; i++ )
    {
        new = (unsigned long) timerList[i];

        if ( new == 0 ){ return (int) i; }
    };

	// fail
    return (int) -1;
}


void timerEnableTextCursor (void)
{
    timerShowTextCursor = TRUE;
}


void timerDisableTextCursor (void)
{
    timerShowTextCursor = FALSE;
}

// systime in ms
unsigned long now (void)
{
    return (unsigned long) get_systime_ms();
}


// fake
void sleep (unsigned long ms)
{
    unsigned long t=1;


    if ( ms == 0 ){
        ms=1;
    }

    // #bugbug
    // Is there a limit?

    t = (unsigned long) ( ms * 512 );

    while (t > 0)
    {
        t--;
    };
}


/*
 * timerInit:
 *     Inicializa o driver de timer.
 *     Inicializa as variáveis do timer.
 *     @todo: KiTimerInit 
 * (unsigned long CallBackExemplo); 
 */

int timerInit (void)
{
    int i=0;

    // g_driver_timer_initialized = FALSE;

// Breaker
    __breaker_timer_initialized = FALSE;


    timerTimer();

    for ( i=0; i<32; i++ ){
        timerList[i] = (unsigned long) 0;
    }


    // timerLock = 0;

    //set handler.
   
    //
    // @todo: Habilitar esse configuração pois é mais precisa.
    //
   
    //config frequências...
    //@todo: Isso poderia ser por último.
    //?? Isso pertence a i386 ??
    //?? Quais máquinas possuem esse controlador ??
    
// #importante
// Começaremos com 100 HZ
// Mas o timer poderá ser atualizado por chamada.
// e isso irá atualizar a variável que inicializamos agora.

    timerInit8253(HZ);

// #todo:
// alocar memoria para a estrutura do timer.
// inicializar algumas variaveis do timer.
// por enquanto estamos usando variaveis globais.
// ?? Não se se ja foi configurado o timer.
// ou devemos chamr init_8253() agora. ou depois.

    //timerCountSeconds = 0;

// Quantum
    set_current_quantum (QUANTUM_MIN);
    set_next_quantum (QUANTUM_MIN);
    set_quantum (QUANTUM_MIN);

// Timeout 
    set_timeout(0);

// Whatchdogs
// Initializing whatchdogs.
// Eles serão zerados pelas interrupções dos dipositivos e
// incrementados pelo timer.
// A condição crítica é alcançar um limite, um timeout.

    ____whatchdog_ps2_keyboard = 0;
    ____whatchdog_ps2_mouse = 0;
    //...


	//Continua...

// breaker
    __breaker_timer_initialized = TRUE;

// Done
    g_driver_timer_initialized = TRUE;
    return 0;
}


/*
 * early_timer_init:
 *     Inicialização prévia do módulo timer.
 *     Uma outra inicialização mais apurada poderá ser feita
 * posteriormente.
 *     Porém ainda existe uma inicialização feita em Assembly
 * quando o kernel é inicialazado.
 */

int early_timer_init (void)
{
    int i=0;

    //g_driver_timer_initialized = FALSE;

// Breaker
    __breaker_timer_initialized = FALSE;

    timerTimer();

    for ( i=0; i<32; i++ ){
        timerList[i] = (unsigned long) 0;
    };


//
// == Hz ============================================
//

// Setup the controller.
// Let's setup the variable sys_time_hz.
// And setup the controler.
// We can use the default variable. 
// See: config.h

    set_systime_hz(DEFAULT_PIT_FREQ);
    timerInit8253 (DEFAULT_PIT_FREQ);

// Quantum
    set_current_quantum (QUANTUM_MIN);
    set_next_quantum (QUANTUM_MIN);
    set_quantum (QUANTUM_MIN);

// Timeout 
    set_timeout(0);

// Whatchdogs
// Initializing whatchdogs.
// Eles serão zerados pelas interrupções dos dipositivos e
// incrementados pelo timer.
// A condição crítica é alcançar um limite, um timeout.
    ____whatchdog_ps2_keyboard = 0;
    ____whatchdog_ps2_mouse = 0;
    //...

    // Continua...

// Breaker
    __breaker_timer_initialized = TRUE;

// Done
    g_driver_timer_initialized = TRUE;
    return 0;
}


/*
struct timer_d *timerObject();
struct timer_d *timerObject()
{
	//@todo: criar uma estrutura e retornar o ponteiro.
	return (struct timer_d *) x;
}
*/


/*
 * create_timer:
 */
// IN: pid, ms, type

struct timer_d *create_timer ( 
    pid_t pid, 
    unsigned long ms, 
    int type )
{
    struct timer_d   *Timer;
    struct process_d *Process;
    struct thread_d  *Thread;

    int ID = -1;  //erro;

    //debug_print("=============\n");
    debug_print("create_timer:\n");
    
    //printf     ("create_timer: pid=%d ms=%d type=%d\n",
    //    pid,ms,type);

    if (pid<0){
        debug_print("create_timer: [FAIL] pid\n");
        return NULL;
    }
    
    Process = (struct process_d *) processList[pid];
    if ( (void*) Process == NULL )
    {
        debug_print("create_timer: [FAIL] Process\n");
        return NULL;
    }

    // Thread de controle.
    //Thread = (struct thread_d *) Process->control;
    Thread = (struct thread_d *) threadList[current_thread];
    if ( (void*) Thread == NULL )
    {
        debug_print("create_timer: [FAIL] Thread\n");
        return NULL;
    }

// limits
// limite de 1 tick.

    // ms
    if (ms < (1000/sys_time_hz) )
    {
        printf ("create_timer: Ajust ms\n");
        ms = (1000/sys_time_hz);
    }

    // type
    if ( type < 1 || type > 10 )
    {
        panic ("create_timer: type fail\n");
        //printf("create_timer: type fail\n");
        //refresh_screen ();
        //return NULL;
    }

//
// Structure
//

    Timer = (void *) kmalloc( sizeof(struct timer_d) );

    if ( (void *) Timer == NULL ){
        panic ("create_timer: Timer fail \n");
        //printf ("create_timer: Timer fail \n");
        //refresh_screen ();
        //return NULL; 

    }else{

        // ??
        // List?
        ID = (int) new_timer_id();

        // Erro ao obter um novo id.
        if (  ID < 0 || ID > 32 ){
            panic ("create_timer: ID fail \n");
            //printf("create_timer: ID fail \n");
            //refresh_screen ();
            //return NULL;

        }else{

            Timer->used = TRUE;
            Timer->magic = 1234;
            Timer->id = ID;

            // ms/(ms por tick)
            Timer->initial_count_down = (unsigned long) ( ms / (1000/sys_time_hz) );
            Timer->count_down = Timer->initial_count_down;

            //1 = one shot 
            //2 = intermitent
            Timer->type = (int) type;
            
            // Pegamos logo acima.
            Timer->process = (struct process_d *) Process;
            Timer->thread  = (struct thread_d *)  Thread;
            Timer->pid     = pid;
            Timer->tid     = current_thread;

            //printf("create_timer: done t={%d} :) \n",
            //    Timer->initial_count_down);

            // Coloca na lista.
            timerList[ID] = (unsigned long) Timer;
        };
    };

    // #debug
    debug_print("create_timer: done\n");
    refresh_screen();

    return (struct timer_d *) Timer;

fail:
    debug_print("create_timer: [FAIL]\n");
    printf     ("create_timer: [FAIL]\n");
    refresh_screen();
    return NULL;
}


