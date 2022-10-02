
// debug.c

#include <kernel.h>


// private
static int __using_serial_debug = FALSE;


//
// Private functions: Prototypes.
//

static int __debug_check_inicialization(void);
static int __debug_check_drivers(void);


// ==============================

// __debug_check_inicialization:
//     Checar se o kernel e os módulos foram inicializados.
//     Checa o valor das flags.
//     Checar todos contextos de tarefas válidas.
// #todo
// Rever esses nomes e etapas.

static int __debug_check_inicialization (void)
{
    int Status = TRUE;

//
// Check phase
//

    if (Initialization.current_phase != 3){
       Status = FALSE;
       printf ("__debug_check_inicialization: Initialization phase = {%d}\n",
           Initialization.current_phase );
       goto fail;
    }

//
// Checkpoints
//

// Hal
    if (Initialization.hal_checkpoint != TRUE){
        Status = FALSE;
        printf ("__debug_check_inicialization: hal\n");
        goto fail;
    }

// Microkernel
    if (Initialization.microkernel_checkpoint != TRUE){
       Status = FALSE;
       printf ("__debug_check_inicialization: microkernel\n");
       goto fail;
    }

// Executive
    if (Initialization.executive_checkpoint != TRUE){
        Status = FALSE;
        printf ("__debug_check_inicialization: executive\n");
        goto fail;
    }

// More?!

// OK
done:
    return (int) Status;
fail:
    die(); 
    return (int) Status;
}


/*
 * __debug_check_drivers:
 *    Checar se os drivers estão inicializados.
 */

static int __debug_check_drivers(void)
{
    int Status = FALSE;

    debug_print("__debug_check_drivers: #todo\n");

    /*
     
	if (g_driver_ps2keyboard_initialized != 1){
	    //erro
	}

	if (g_driver_ps2mouse_initialized != 1){
	    //erro
	}


	if (g_driver_hdd_initialized != 1){
	    //erro
	}

    if (g_driver_pci_initialized != 1){
	    //erro
	}
	
    if (g_driver_rtc_initialized != 1){
	    //erro
	}
	
    if (g_driver_timer_initialized != 1){
	    //erro
	}

    */

    return (int) Status;
}

// ==============================

int is_using_serial_debug(void)
{
    return (int) __using_serial_debug;
}

void enable_serial_debug(void)
{
    __using_serial_debug = TRUE;
}

void disable_serial_debug(void)
{
    __using_serial_debug = FALSE;
}

void debug_print ( char *data )
{
    register int i=0;

// Are we using this debug method 
// at this moment or not?

    int using_serial_debug = (int) is_using_serial_debug();

    if ( using_serial_debug == FALSE ){
        return;
    }

// Is it fully initialized?

    if( Initialization.serial_log != TRUE ){
        return;
    }

    if ( (void *) data == NULL ){ return; }
    if (*data == 0)             { return; }

    for ( i=0; data[i] != '\0'; i++ )
    {
        serial_write_char ( COM1_PORT, data[i] );
    };
}


// We will use this function to track 
// the main kernel initialization progress.
// It will print into the serial port for now.

void PROGRESS( char *string )
{
    if( Initialization.serial_log != TRUE )
        return;

    if( (void*) string == NULL ){
        return;
    }

    if(*string == 0){
        return;
    }

    // #todo
    // Select the available method.
    // switch(...

    debug_print("\n");
    debug_print(string);
}


/*
 * debug:
 *     Checa por falhas depois de cumpridas as 
 *     três fases de inicialização.
 */
// #bugbug
// Será que o output está realmente disponível nesse momento ?!

int debug (void)
{
    int Status = -1; 

// Checa inicialização. 
// Fases, variáveis e estruturas.

    Status = (int) __debug_check_inicialization();

    if (Status == TRUE){
        panic ("debug: __debug_check_inicialization fail\n");
    }

    // 'processor' struct.

    if ( (void *) processor == NULL ){
        panic ("debug: processor struct fail\n");
    }

// Check drivers status. 
// ( Ver se os principais drivers estão inicializados )
    __debug_check_drivers();

/*
 * @todo: 
 *     Checar se existe componentes do sistema como mbr, root, fat 
 * e arquivos e programas básicos do sistema.
 */
 
/* 
 * @todo: 
 *     Checar por falhas no sistema de arquivos.
 */
 
 
	/*
     * @todo:	
	 *     Checar por falhas nas estruturas de tarefas.
	 */

	//...


    // printf ("debug: Done.\n");

    return 0; 
}

// debug_breakpoint:
//     Para a execução do sistema.
//     @todo: isso pode ir para o arquivo debug.c.

void debug_breakpoint (void)
{
    printf ("debug_breakpoint:\n");
    die();
}


// Compute checksum
// IN: address, lenght.
// see: kstring.c
unsigned long 
debug_compute_checksum ( 
    unsigned char *buffer, 
    unsigned long lenght )
{
    unsigned long res=0;
    
    res = 
        (unsigned long) string_compute_checksum(
                            (unsigned char *) buffer,
                            (unsigned long) lenght );
    return (unsigned long) res;
}

//
// End
//

