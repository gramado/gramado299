/*
 * File: main.c
 *     Simple text editor for Gramado Window Server.
 * History:
 *     2020 - Created by Fred Nora.
 */
// Connecting via AF_INET.
// tutorial example taken from. 
// https://www.tutorialspoint.com/unix_sockets/socket_server_example.htm
/*
    To make a process a TCP server, you need to follow the steps 
    given below −
    Create a socket with the socket() system call.
    Bind the socket to an address using the bind() system call. 
    For a server socket on the Internet, an address consists of a 
    port number on the host machine.
    Listen for connections with the listen() system call.
    Accept a connection with the accept() system call. 
    This call typically blocks until a client connects with the server.
    Send and receive data using the read() and write() system calls.
*/
// See:
// https://wiki.osdev.org/Message_Passing_Tutorial
// https://wiki.osdev.org/Synchronization_Primitives
// ...

// rtl
#include <types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <packet.h>
#include <rtl/gramado.h>
// The client-side library.
#include <gws.h>
// Internal
#include <editor.h>

#define WINDOW_COUNT_MAX  1024
unsigned long windowList[WINDOW_COUNT_MAX];


unsigned long gScreenWidth=0;
unsigned long gScreenHeight=0;


//
// windows
//

// private
static int main_window = 0;
static int addressbar_window = 0;
static int client_window = 0;
static int savebutton_window = 0;




// #todo
// int button_list[8];

//
// cursor
//

static int cursor_x = 0;
static int cursor_y = 0;
static int cursor_x_max = 0;
static int cursor_y_max = 0;

static int blink_status=FALSE;


//
// tmp input pointer.
//

// #todo
// we will copy all the iput support from the other editor.
// for now we will use this tmp right here.

int tmp_ip_x=8;
int tmp_ip_y=8;

//
// == ports ====================================
//

#define PORTS_WS 4040
#define PORTS_NS 4041
#define PORTS_FS 4042
// ...


#define IP(a, b, c, d) (a << 24 | b << 16 | c << 8 | d)


//prototype
static int 
editorProcedure(
    int fd, 
    int event_window, 
    int event_type, 
    unsigned long long1, 
    unsigned long long2 );

static void update_clients(int fd);

static int editor_init_windows(void);
static int editor_init_globals(void);

// ============

static void update_clients(int fd)
{
    if (fd<0)
        return;

//#todo: test buttons validation.
    gws_redraw_window(fd, addressbar_window, TRUE);
    gws_redraw_window(fd, savebutton_window, TRUE);
    gws_redraw_window(fd, client_window,     TRUE);

//#test
    gws_set_focus(fd,client_window);
}

static int editor_init_globals(void)
{
    gScreenWidth  = (unsigned long) gws_get_system_metrics(1);
    gScreenHeight = (unsigned long) gws_get_system_metrics(2);
    //...
    return 0;
}


static int editor_init_windows(void)
{
    int i=0;
    for (i=0; i<WINDOW_COUNT_MAX; i++){
        windowList[i] = 0;
    };
    
    return 0;
}


//char *hello = "Hello there!\n";

/*
#define IP(a, b, c, d) (a << 24 | b << 16 | c << 8 | d)
struct sockaddr_in addr = {
    .sin_family = AF_INET,
    .sin_port   = 7548, 
    .sin_addr   = IP(192, 168, 1, 79),
};
*/


// Quem deveria fazer isso seria o window server
// escrevendo na janela com foco de entrada 
// e com as características de edição configuradas pra ela.
// Ou ainda uma biblioteca client-side.

void 
editorDrawChar( 
    int fd,
    int ch)
{
    int pos_x=0;
    int pos_y=0;

// Get saved value
    pos_x = (int) (cursor_x & 0xFFFF);
    pos_y = (int) (cursor_y & 0xFFFF);

    if ( pos_x < 0 ){ pos_x = 0; }
    if ( pos_y < 0 ){ pos_y = 0; }

// End of line
    if ( pos_x >= cursor_x_max )
    {
        pos_x = 0;
        pos_y++;
    }

// Last line
// #todo: scroll
    if ( pos_y >= cursor_y_max )
    {
        pos_y = cursor_y_max-1;
    }

// Save cursor
    cursor_x = pos_x;
    cursor_y = pos_y;

// Draw
// Calling the window server for drawing the char.

    gws_draw_char ( 
        fd, 
        client_window, 
        (cursor_x*8), 
        (cursor_y*8), 
        COLOR_BLACK, 
        ch );

    // increment
    cursor_x++;
}


void
editorSetCursor( 
    int x,
    int y )
{
    if (cursor_x >= 0 && 
        cursor_x < cursor_x_max)
    {
        cursor_x = x;
    }

    if (cursor_y >= 0 && 
        cursor_y < cursor_y_max)
    {
        cursor_y = y;
    }
}


// local
static int 
editorProcedure(
    int fd, 
    int event_window, 
    int event_type, 
    unsigned long long1, 
    unsigned long long2 )
{
    if (fd<0){
        return -1;
    }
    if (event_window<0){
        return -1;
    }
    if (event_type < 0){
        return -1;
    }

// Event type:

    switch(event_type){

    case 0:
        //#debug
        //printf("EDITOR: Null event!\n");
        return 0;
        break;

    //36
    case MSG_MOUSERELEASED:
        if( event_window == addressbar_window ||
            event_window == client_window )
        {
            //printf("editor.bin: MSG_MOUSERELEASED\n");
            //gws_redraw_window(fd, event_window, TRUE);
            
            if(event_window == client_window)
            {
                // Refresh?
                gws_draw_char (
                    (int) fd,              // fd
                    (int) event_window,    // wid
                    (unsigned long) long1, // left
                    (unsigned long) long2, // top
                    (unsigned long) COLOR_BLACK,
                    (unsigned long) '.' );
            }
            return 0;
        }

        if(event_window == savebutton_window)
            printf("EDITOR: Save!\n");
        
        return 0;
        break;

    // Repainting all the client windows.
    // If the event window is the main window, so
    // redraw everyone.
    case MSG_PAINT:
        if ( event_window == main_window ){
            update_clients(fd);
            return 0;
        }
        break;

    // test
    //case 8888:
        //break;

    //...
    
    default:
        return -1;
        break;
    };

    return -1;
}



int main( int argc, char *argv[] )
{
    struct gws_display_d *Display;
    int client_fd = -1;

    //debug_print ("EDITOR.BIN: Initializing\n");


/*
// #test
// OK!
    int i=0;
	for (i = 1; i < argc; i++)
		if (strcmp("--test--", argv[i]) == 0)
			printf("TEST\n");
*/


// global: Cursor
    cursor_x = 0;
    cursor_y = 0;

// ============================
// Open display.
// IN: 
// hostname:number.screen_number

    Display = 
        (struct gws_display_d *) gws_open_display("display:name.0");

    if ( (void*) Display == NULL )
    {
        debug_print ("editor: Couldn't open display\n");
        printf      ("editor: Couldn't open display\n");
        exit(1);
    }

    client_fd = (int) Display->fd;

    if ( client_fd <= 0 )
    {
        debug_print ("editor: bad fd\n");
        printf      ("editor: bad fd\n");
        exit(1);
    }

// =====================================================

// Device info
    unsigned long w = gws_get_system_metrics(1);
    unsigned long h = gws_get_system_metrics(2);
    if ( w == 0 || h == 0 ){
        printf ("editor: w h \n");
        exit(1);
    }

// Tamanho da janela.
    unsigned long w_width  = (w>>1);
    unsigned long w_height = (h - 100); //(h>>1);

// original
    unsigned long viewwindowx = ( ( w - w_width ) >> 1 );
    unsigned long viewwindowy = ( ( h - w_height) >> 1 ); 

    // test1: Erro de posicionamento.
    //unsigned long viewwindowx = 580;
    //unsigned long viewwindowy = ( ( h - w_height) >> 1 ); 

    // test2: Erro de posicionamento.
    //unsigned long viewwindowx = ( ( w - w_width ) >> 1 );
    //unsigned long viewwindowy = 400; 

    // test3: top/left
    //unsigned long viewwindowx = 0;
    //unsigned long viewwindowy = 0; 

// #hackhack
// @media
// Se a tela for pequena demais para os dias de hoje. hahaha

    // Position and dimensions.
    if (w == 320){
        viewwindowx = 0;
        viewwindowy = 0;
        w_width = w;
        w_height = h;
    }

// Cursor limits based on the window size.
    cursor_x = 0;
    cursor_y = 0;
    cursor_x_max = ((w_width/8)  -1);
    cursor_y_max = ((w_height/8) -1);


// Creating the main window.
// style: 
// 0x0001 = maximized 
// 0x0002 = minimized 
// 0x0004 = fullscreen
// 0x0008 = use statusbar
// ...

    main_window = 
        (int) gws_create_window (
                  client_fd,
                  WT_OVERLAPPED, 1, 1, "EDITOR", 
                  viewwindowx, viewwindowy, w_width, w_height,
                  0, 
                  0x0000,  
                  COLOR_RED,   // #todo: client bg. Not implemented. 
                  COLOR_GRAY );

    if (main_window < 0){
        debug_print("Editor: main_window fail\n"); 
        exit(1);
    }

// Labal.
// Text inside the main window.
// Right below the title bar.
// Right above the client window.
     gws_draw_text (
        (int) client_fd,      // fd,
        (int) main_window,    // window id,
        (unsigned long) 2,          // l 
        (unsigned long) 4 +(24/3),  // t
        (unsigned long) COLOR_BLACK,
        " Name: ");

// Address bar - (edit box)
// Inside the main window.
// se a janela mae é overlapped, 
// então estamos relativos à sua área de cliente.
    addressbar_window = 
        (int) gws_create_window (
                  client_fd,
                  WT_EDITBOX,1,1,"address-bar",
                  (( w_width/8 )*2),    //l
                  4,                    //t
                  (( w_width/8 )*3), 
                  24,    
                  main_window, 
                  0, COLOR_WHITE, COLOR_WHITE );

    if (addressbar_window < 0){
        //debug_print("editor: addressbar_window fail\n");
    }

// Text inside the address bar.
    if (addressbar_window > 0)
    {
        gws_draw_text (
            (int) client_fd,            // fd
            (int) addressbar_window,    // window id
            (unsigned long) 8,          // left
            (unsigned long) 8,          // top
            (unsigned long) COLOR_BLACK,
            "text.txt");
     }

// The [Save] button.
// inside the main window.

    savebutton_window = 
        (int) gws_create_window ( 
                  client_fd,
                  WT_BUTTON,1,1,"Save",
                  (( w_width/8 )*6),  //l 
                  4,                  //t
                  (( w_width/8 )*1), 
                  24,
                  main_window, 0, COLOR_GRAY, COLOR_GRAY );

    if (savebutton_window < 0){
        //debug_print("editor: savebutton_window fail\n"); 
    }

//
// == Client window =======================
//


/*

 // #todo: Get the client window's info.
 // see: the same in terminal.bin appication.

    struct gws_window_info_d *wi;
    wi = (void*) malloc( sizeof( struct gws_window_info_d ) );
    if( (void*) wi == NULL )
    {
        printf("terminal: wi\n");
        while(1){}
    }
    //IN: fd, wid, window info structure.
    gws_get_window_info(
        client_fd, 
        main_window,   // The app window.
        (struct gws_window_info_d *) wi );
*/

// (Editbox)
// Client window (White)
// Inside the mainwindow.
// Lembre-se que temos uma status bar.

// left:
    unsigned long cw_left = 4;
// top: pad | address bar | pad
    unsigned long cw_top = 4 +24 +4;
// width: Width - borders.
    unsigned long cw_width = ((w_width/8)*7);
// height:
// #bugbug:
// We gotta get the client window values.
    unsigned long cw_height = ((w_height/8)*6);

    client_window = 
        (int) gws_create_window ( 
                  client_fd,
                  WT_EDITBOX_MULTIPLE_LINES, 1, 1, "Client",
                  cw_left, cw_top, cw_width, cw_height,
                  main_window, 0, COLOR_WHITE, COLOR_WHITE );

    if (client_window < 0){
        //debug_print("editor: client_window fail\n"); 
    }

// Show main window.
    gws_refresh_window (client_fd, main_window);

// ============================================
// focus
// editbox
// Setting the input focus on a given window.
// Input
// #focus
// Well, the editor.bin application is not receiving
// the input ... so, i guess the window server
// is printing the chars into the window with focus.

    gws_async_command(
         client_fd,
         9,             // set focus
         client_window,
         client_window );


//
// Event loop
//

    struct gws_event_d lEvent;
    lEvent.used = FALSE;
    lEvent.magic = 0;
    lEvent.type = 0;
    lEvent.long1 = 0;
    lEvent.long2 = 0;

    struct gws_event_d *e;

// loop
// The server will return an event from the client's event queue.
// Call the local window procedure if a valid event was found.
// #todo: 
// Por enquanto, a rotina no servidor somente lida com 
// eventos na janela com foco de entrada.
// Talvez a ideia é lidar com eventos em todas as janelas
// do processo cliente.

    //Display->running = TRUE;

    while (1)
    {
        //if ( Display->running != TRUE )
            //break;

        e = 
        (struct gws_event_d *) gws_get_next_event(
                                   client_fd, 
                                   (struct gws_event_d *) &lEvent );

        if ( (void *) e != NULL )
        {
            //if( e->used == TRUE && e->magic == 1234 )
            if (e->magic == 1234){
                editorProcedure(
                    client_fd, e->window, e->type, e->long1, e->long2 );
            }
        }
    };


// HANG
    while(1)
    {
        rtl_yield();
    };


/*
    int C=0;
    //char data[2];
    //int nread=0;

    //fputc('A',stdin);
    //fputs("This is a string in stdin",stdin);

    rewind(stdin);

    while (1){
        C=fgetc(stdin);
        if(C>0){
            editorProcedure( 
                client_fd,     // socket
                NULL,          // opaque window object
                MSG_KEYDOWN,   // message code
                C,             // long1 (ascii)
                C );           // long2 (ascii)
        }
    };
*/

//==============================================


//
// loop
//

/*
//=================================
// Set foreground thread.
// Get events scanning a queue in the foreground queue.
    rtl_focus_on_this_thread();
    
    while (1){
        if ( rtl_get_event() == TRUE )
        {  
            editorProcedure( 
                client_fd,
                (void*) RTLEventBuffer[0], 
                RTLEventBuffer[1], 
                RTLEventBuffer[2], 
                RTLEventBuffer[3] );
        }
    };

//=================================
*/





    // #importante
    // Se não usarmos o loop acima, então podemos pegar
    // as mensagens do sistema....
    // O ws pode mandar mensagens de sistema para o
    // wm registrado.


    /*
    struct gws_event_d *Event;
     
    for(;;){
        
        Event = (struct gws_event_d *) gws_next_event();
        
        if (Event.type == 0){
           gws_debug_print("editor: event 0\n");
        
        }else if (Event.type == 1){
           gws_debug_print("editor: event 1\n");
        
        }else if (Event.type == 2){
           gws_debug_print("editor: event 2\n");
        
        }else{
           gws_debug_print("editor: Not valid event!\n");
        };
    };
    */


// exit
    //close (client_fd);
    debug_print ("editor: bye\n"); 
    printf      ("editor: bye\n");

    return 0;
}


//
// End.
//








