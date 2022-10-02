// init process
// GWS.BIN

#include <types.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <rtl/gramado.h>


#define VK_RETURN    0x1C
#define VK_TAB       0x0F


static isTimeToQuit = FALSE;

// private functions: prototypes;
static void initPrompt(void);
static int initCompareString(void);


// ====================

static void initPrompt(void)
{
    int i=0;

    // Clean prompt buffer.
    
    for ( i=0; i<PROMPT_MAX_DEFAULT; i++ ){ prompt[i] = (char) '\0'; };
    
    prompt[0] = (char) '\0';
    prompt_pos    = 0;
    prompt_status = 0;
    prompt_max    = PROMPT_MAX_DEFAULT;  

    // Prompt
    printf("\n");
    putc('$',stdout);
    putc(' ',stdout);
    fflush(stdout);
}


static int initCompareString(void)
{
    char *c;
    c = prompt;
    if( *c == '\0' )
        goto exit_cmp;
    
    //LF
    printf("\n");
    
    if( strncmp(prompt,"help",4) == 0 )
    {
        printf ("HELP:\n");
        printf("[control + f9] to open the kernel console\n");
        printf ("Commands: help, reboot, shutdown ...\n");
        goto exit_cmp;
    }

    if( strncmp(prompt,"reboot",6) == 0 )
    {
        printf ("REBOOT\n");
        rtl_reboot();
        goto exit_cmp;
    }

    if( strncmp(prompt,"shutdown",8) == 0 )
    {
        printf ("SHUTDOWN\n");
        rtl_clone_and_execute("shutdown.bin");
        goto exit_cmp;
    }

    if( strncmp(prompt,"ws",2) == 0 )
    {
        printf ("~WS\n");
        rtl_clone_and_execute("gwssrv2.bin");
        goto exit_cmp;
    }

    if( strncmp(prompt,"client",6) == 0 )
    {
        printf ("~Client\n");
        rtl_clone_and_execute("terminal.bin");
        isTimeToQuit = TRUE;
        goto exit_cmp;
    }

    // ...

    printf ("Command not found\n");

exit_cmp:
    initPrompt();
    return 0;
}


int main( int argc, char **argv)
{
    //#todo
    //printf()

//
// Interrupts
//

// The taskswithing will not work without this.

    asm ("int $199 \n");


// interrupts
// Unlock the taskswitching support.
// Unlock the scheduler embedded into the base kernel.
// Only the init process is able to do this.

    //gws_debug_print ("gws.bin: Unlock taskswitching and scheduler \n");
    //printf          ("gws.bin: Unlock taskswitching and scheduler \n");
    gramado_system_call (641,0,0,0);
    gramado_system_call (643,0,0,0);

    printf(":: Gramado OS ::\n");

    int C=0;

    initPrompt();

    while(1)
    {
        if( isTimeToQuit == TRUE ){
            break;
        }
        C = (int) fgetc(stdin);
        if( C == VK_RETURN )
            initCompareString();
        if( C >= 0x20 && C <= 0x7F )
        {
            input(C);
            printf("%c",C);
            fflush(stdout);
        }
    };

    printf("~Quit\n");

// hang
    while (TRUE){
        rtl_yield();
    };

    return 0;
}

