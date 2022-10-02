
// kobject.h
// We also have the object types used by the file system.
// See: gobject.h

#ifndef __KOBJECT_H
#define __KOBJECT_H    1

// #todo
// We need an index for serial devices.
// /dev/ttyS...

//Precisamos de um pouco de ordem nisso.
//separar por grupos.
//Talvez usarmos os prefixos e as pastas do kernel.

typedef enum {

    ObjectTypeNull,               //0 Null. (Free)

    // fs/
    ObjectTypeDirectory,          // diretório de arquivos
    ObjectTypeFile,               // (Regular file)
    ObjectTypePipe,               //
    ObjectTypeFileSystem,         // <<<< This file represents a 'File System'.
    
    // metafile just like GRAMADO or BRAMADO.
    // or even a hidden lba with no entry in the directory
    ObjectTypeMetafile,

    // hwi/
    
    //ObjectTypeSerialPort,    //COM ports.
    
    ObjectTypeDevice,         // device ??
    ObjectTypeDisk,           //
    ObjectTypeDiskInfo,       // disk info,
    ObjectTypeDiskPartition,  // info struct    
    ObjectTypeVolume,         // disk volume 
    ObjectTypeTerminal,       // 
    ObjectTypeLine,           //
    ObjectTypePTMX,           // <<<< This file represents a pty multiplexer.  
    ObjectTypeTTY,            //
    ObjectTypePTY,            //
    ObjectTypeVirtualConsole, //
    ObjectTypeCharDev,        //
    ObjectTypeBlockDev,       //
    ObjectTypeNetworkDev,     //
    ObjectTypeVideo,          // video
    ObjectTypeCpu,           
    ObjectTypeDma,  
    ObjectTypeProcessor,      // processor
    ObjectTypeProcessorBlock, // processor block 
    ObjectTypeMemory,         // memory card info.
    ObjectTypePIC,            // pic controller.
    ObjectTypeTimer,          // PIT timer
    ObjectTypeRTC,            // rtc controller, clock
    ObjectTypeKeyboard,       // keyboard device.
    ObjectTypeMouse,          // mouse device.
    ObjectTypeComputer,       //
    ObjectTypeCpuRegister,    //
    ObjectTypeGDT,            //
    ObjectTypeLDT,            //
    ObjectTypeIDT,            //
    ObjectTypeTSS,            //
    ObjectTypePort,           // (i/o port) generic
    ObjectTypeController,     //
    ObjectTypePciDevice,        // PCI device info generic
    ObjectTypePciDeviceDriver,  // PCI device driver info.
    ObjectTypeScreen,           // Screen.

    // klib/
    ObjectTypeSignal,     // unix signal object ??

    // net/
    ObjectTypeHostInfo,     // HostInfo. 
    ObjectTypeProtocol,     // protocol
    ObjectTypeSocket,       // sockets.
    ObjectTypeIpAddress,    // IP address.
    ObjectTypeMacAddress,   // MAC Address.
    ObjectTypeChannel,      // channel = two sockets.

    // ps/ 
    ObjectTypeProcess,            // Process.
    ObjectTypeThread,             // Thread.
    ObjectTypePageDirectory,      // page directory
    ObjectTypePageTable,          // page table
    ObjectTypePageFrame,          // page frame
    ObjectTypeFramePool,          // frame pool
    ObjectTypeASpace,             // Address Space. (memory address)
    ObjectTypeDSpace,             // Disk Space.
    ObjectTypeBank,               // Bank. (banco de dados).
    ObjectTypeHeap,               // heap
    ObjectTypeIoBuffer,           // i/o buffer
    ObjectTypeProcessMemoryInfo,  // process memory info
    ObjectTypePhysicalMemoryInfo, // physical memory info
    ObjectTypeMemoryInfo,         // memory info
    ObjectTypeMemoryBlock,        // memory block. Usado para swap.
    ObjectTypeSemaphore,          // Semaphore
    ObjectTypeMutex, 

    // security/
    ObjectTypeUser,               // user ??
    ObjectTypeUserInfo,           // userinfo ??
    ObjectTypeGroup,              // user group
    ObjectTypeUserSession,        // User session
    ObjectTypeRoom,               // room = (window station), desktop pool.
    ObjectTypeDesktop,            // desktop.
    ObjectTypeToken,              // Token de acesso à objetos. (access token)

    // ws/
    ObjectTypeWindow,             // window 
    ObjectTypeRectangle,          // rectangle
    ObjectTypeMenu,               // menu
    ObjectTypeMenuItem,           // menuitem
    ObjectTypeButton,    
    ObjectTypeGrid,     
    ObjectTypeColorScheme,  
    ObjectTypeFont,  
    ObjectTypeIcon,  
    ObjectTypeRGBA,               // rgba 
    ObjectTypeWindowProcedure,    // window procedure
    ObjectTypeCursor, 
    ObjectTypeMessage,            // system message

    //kernel/
    ObjectTypeRequest,        // request de kernel.
    ObjectTypeKM,             // ??
    ObjectTypeUM,             // ??

// para validação de objetos não especificados 
// ou até nulos, talvez. :)
    ObjectTypeGeneric       


}object_type_t;


//
// Object class.
//

typedef enum {
    ObjectClassKernelObjects,  // Kernel Objects.
    ObjectClassUserObjects,    // User Objects.
    ObjectClassGuiObjects,     // Graphical User Interface Objects.
    ObjectClassCliObjects,     // Command Line Interface Objects.
    //...
}object_class_t;






/*
 * struct object_d:
 *     Object controle ...
 *     Estrutura para controle de objetos.
 *     Isso deve ficar no início de cada estrutura 
 *     para controlar a utilização do objeto por parte 
 * dos processos e threads.
 */

struct object_d 
{
    // haha !!
    // Objeto do tipo objeto.
    object_type_t  objectType;
    object_class_t objectClass;

    int id; 
    int used;  
    int magic; 

    char *path;             // '/root/user/(name)'
    char __objectname[64];  // HOSTNAME_BUFFER_SIZE
    size_t objectName_len;  // len 

    //Lista de processos que possuem o objeto.
    int pidList[32];
    int pidCount;
    int currentPID;

    //endereços
    unsigned long obj_address;
    unsigned long obj_procedure;

// Status do objeto.

    int token;    //algum processo 'pegou' o objeto e esta usando.
    int task_id;  //id da tarefa que pegou o processo.

    //int signaled;
    //struct process_d *waitListHead;

    //channel	
    //process
    //thread
    //window
    //continua...

    //struct object_d *next;
};


// ??
// Repensar isso.
// #bugbug: Muito espaço. (#todo: deprecated this)
//struct object_d objects_km[256+1];  //objetos em kernel mode.
//struct object_d objects_um[256+1];  //objetos em user mode. 
//struct object_d objects_gui[256+1]; //objetos gráficos. 


// Se o gerenciador de recursos foi inicializado.
int g_object_manager_status;

//id do objeto atual
int g_current_object;

//id da lista ao qual o objeto atual pertence.
// object_km, object_um, object_gui. 
int g_current_list;


//
// prototypes ==========
//

#endif    



