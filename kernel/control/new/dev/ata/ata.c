
// ata.c
// ata handshake.
// write command and read status.
// Created by Nelson Cole.
// A lot of changes by Fred Nora.

#include <kernel.h>  

// base address 
// BAR0 is the start of the I/O ports used by the primary channel.
// BAR1 is the start of the I/O ports which control the primary channel.
// BAR2 is the start of the I/O ports used by secondary channel.
// BAR3 is the start of the I/O ports which control secondary channel.
// BAR4 is the start of 8 I/O ports controls the primary channel's Bus Master IDE.
// BAR4 + 8 is the Base of 8 I/O ports controls secondary channel's Bus Master IDE.

unsigned long ATA_BAR0_PRIMARY_COMMAND_PORT=0;    // Primary Command Block Base Address
unsigned long ATA_BAR1_PRIMARY_CONTROL_PORT=0;    // Primary Control Block Base Address
unsigned long ATA_BAR2_SECONDARY_COMMAND_PORT=0;  // Secondary Command Block Base Address
unsigned long ATA_BAR3_SECONDARY_CONTROL_PORT=0;  // Secondary Control Block Base Address
unsigned long ATA_BAR4=0;  // Legacy Bus Master Base Address
unsigned long ATA_BAR5=0;  // AHCI Base Address / SATA Index Data Pair Base Address


/* 
 * Obs:
 * O que segue são rotinas de suporte ao controlador IDE.
 */


// O próximo ID de unidade disponível.
static uint32_t __next_sd_id = 0; 

//
// == Private functions: prototypes ==============
//


static void __local_io_delay (void);
static void __ata_pio_read ( void *buffer, int bytes );
static void __ata_pio_write ( void *buffer, int bytes );
static unsigned char __ata_config_structure(char nport);
static void __set_ata_addr (int channel);

// =======================================================

static void __local_io_delay (void)
{
    asm ("xorl %%eax, %%eax" ::);
    asm ("outb %%al, $0x80"  ::);
    return;
}

// low level worker
static void __ata_pio_read ( void *buffer, int bytes )
{

// #todo:
// avoid this for compatibility with another compiler.

    asm volatile  (\
        "cld;\
        rep; insw"::"D"(buffer),\
        "d"(ata.cmd_block_base_address + ATA_REG_DATA),\
        "c"(bytes/2) );
}

// low level worker
static void __ata_pio_write ( void *buffer, int bytes )
{

// #todo:
// avoid this for compatibility with another compiler.

    asm volatile  (\
        "cld;\
        rep; outsw"::"S"(buffer),\
        "d"(ata.cmd_block_base_address + ATA_REG_DATA),\
        "c"(bytes/2) );
}

// Global
void ata_wait (int val)
{
    if ( val <= 100 )
    {
        val = 400;
    }

    val = ( val/100 );

    while (val--){
        io_delay();
    };
}


// Global
// Forces a 400 ns delay.
// Waste some time.
void ata_delay (void)
{
    int i=0;

    for (i=0; i < 5; i++){
        __local_io_delay();
    };
}


// #bugbug
// Lê o status de um disco determinado, se os valores na estrutura 
// estiverem certos.
// #todo: return type.

unsigned char ata_status_read (void)
{
    return in8( ata.cmd_block_base_address + ATA_REG_STATUS );
}


void ata_cmd_write (int cmd_val)
{
    // no_busy 
    ata_wait_not_busy();

    out8 ( 
        (unsigned short) ((ata.cmd_block_base_address + ATA_REG_CMD) & 0xFFFF), 
        (unsigned char) (cmd_val & 0xFF) );

// #todo
// Esperamos 400ns
    ata_wait(400);  
}


void ata_soft_reset (void)
{
    unsigned char data = (unsigned char) in8( ata.ctrl_block_base_address );

    out8( 
        (unsigned short) ata.ctrl_block_base_address, 
        (unsigned char) (data | 0x4) );

    out8( 
        (unsigned short) ata.ctrl_block_base_address, 
        (unsigned char) (data & 0xFB) );
}


unsigned char ata_wait_drq (void)
{
    while (!(ata_status_read() & ATA_SR_DRQ))
        if ( ata_status_read() & ATA_SR_ERR )
            return 1;

    return 0;
} 


unsigned char ata_wait_no_drq (void)
{

    while ( ata_status_read() & ATA_SR_DRQ )
        if ( ata_status_read() & ATA_SR_ERR )
            return 1;

    return 0;
}


unsigned char ata_wait_busy (void)
{

    while (!(ata_status_read() & ATA_SR_BSY ))
        if ( ata_status_read() & ATA_SR_ERR )
            return 1;

    return 0;
}


// TODO: 
// Ao configurar os bits BUSY e DRQ devemos verificar retornos de erros.

unsigned char ata_wait_not_busy (void)
{

    while ( ata_status_read() & ATA_SR_BSY )
        if ( ata_status_read() & ATA_SR_ERR )
            return 1;

    return 0;
}


// local worker
static void __set_ata_addr (int channel)
{

// #todo
// filtrar limites.

    //if( channel<0 )
        //panic()

    switch (channel){

    case ATA_PRIMARY:
        ata.cmd_block_base_address  = ATA_BAR0_PRIMARY_COMMAND_PORT;
        ata.ctrl_block_base_address = ATA_BAR1_PRIMARY_CONTROL_PORT;
        ata.bus_master_base_address = ATA_BAR4;
        break;

    case ATA_SECONDARY:
        ata.cmd_block_base_address  = ATA_BAR2_SECONDARY_COMMAND_PORT;
        ata.ctrl_block_base_address = ATA_BAR3_SECONDARY_CONTROL_PORT;
        ata.bus_master_base_address = ATA_BAR4 + 8;
        break;

    //default:
        //PANIC
        //break;
    };
}


// local
// __ata_config_structure:
// Set up the ata.xxx structure.
// #todo: Where is that structure defined?
// See: ata.h
// De acordo com a porta, saberemos se é 
// primary ou secondary e se é
// master ou slave.

static unsigned char __ata_config_structure (char nport)
{

// todo
// filtrar limits.

    switch (nport){

    case 0:
        ata.channel = ATA_PRIMARY; //0;  // primary
        ata.dev_num = ATA_MASTER;  //0;  // not slave ATA_MASTER
        break;

    case 1: 
        ata.channel = ATA_PRIMARY; //0;  // primary
        ata.dev_num = ATA_SLAVE;   //1;  // slave    ATA_SLAVE
        break;

    case 2:
        ata.channel = ATA_SECONDARY;  //1;  // secondary
        ata.dev_num = ATA_MASTER;     //0;  // not slave ATA_MASTER
        break;

    case 3:
        ata.channel = ATA_SECONDARY;  //1;  // secondary
        ata.dev_num = ATA_SLAVE;      //1;  // slave    ATA_SLAVE
        break;

    default:
        // panic ?
        printk ("Port %d not used\n", nport );
        return -1;
        break;
    };

    // local worker.
    __set_ata_addr (ata.channel);

    return 0;
}

// atapi_pio_read:

inline void atapi_pio_read ( void *buffer, uint32_t bytes )
{

// #todo
// Avoid this for compatibility with another compiler.

    asm volatile  (\
        "cld;\
        rep; insw"::"D"(buffer),\
        "d"(ata.cmd_block_base_address + ATA_REG_DATA),\
        "c"(bytes/2) );
}


void ata_set_boottime_ide_port_index(unsigned int port_index)
{
    g_boottime_ide_port_index = (int) port_index;
}


int ata_get_boottime_ide_port_index(void)
{
    return (int) g_boottime_ide_port_index;
}


void ata_set_current_ide_port_index(unsigned int port_index)
{
    g_current_ide_port_index = (int) port_index;
}


int ata_get_current_ide_port_index(void)
{
    return (int) g_current_ide_port_index;
}


int 
ata_ioctl ( 
    int fd, 
    unsigned long request, 
    unsigned long arg )
{
    debug_print("ata_ioctl: [TODO] \n");
    return -1;
}


// ata_initialize:
// Inicializa o IDE e mostra informações sobre o disco.
// Sondando na lista de dispositivos encontrados 
// pra ver se tem algum controlador de disco IDE.
// IN: ?
// Configurando flags do driver.
// FORCEPIO = 1234
// Called by ataDialog in atainit.c

int ata_initialize ( int ataflag )
{
    int Status = -1;  //error
    int Value = -1;

    // iterator.
    int iPortNumber=0;

    unsigned char bus=0;
    unsigned char dev=0;
    unsigned char fun=0;


    debug_print("ata_initialize: todo \n");

    // Setup interrupt breaker.
    //debug_print("ata_initialize: Turn on interrupt breaker\n");
    __breaker_ata1_initialized = FALSE;
    __breaker_ata2_initialized = FALSE;


// A estrutura ainda nao foi configurada.
    ata.used = FALSE;
    ata.magic = 0;

//
// ======================================
//

// #importante 
// HACK HACK
// Usando as definições feitas em config.h
// até que possamos encontrar dinamicamente 
// o canal e o dispositivo certos.
// __IDE_PORT indica qual é o indice de porta.
// Configumos o atual como sendo o mesmo
// usado durante o boot.
// #todo
// Poderemos mudar o atual conforme nossa intenção
// de acessarmos outros discos.

// See: config.h

    unsigned int boottime_ideport_index = __IDE_PORT;
    unsigned int current_ideport_index = __IDE_PORT;

    ata_set_boottime_ide_port_index(boottime_ideport_index);
    ata_set_current_ide_port_index(current_ideport_index);

//
// ============================================
//

// ??
// Configurando flags do driver.
// FORCEPIO = 1234

    ATAFlag = (int) ataflag;

//
// Messages
//

//#ifdef KERNEL_VERBOSE
    //printf ("ata_initialize:\n");
    //printf ("Initializing IDE/AHCI support ...\n");
//#endif

// #test
// Sondando na lista de dispositivos encontrados 
// pra ver se tem algum controlador de disco IDE.
// #importante:
// Estamos sondando uma lista que contruimos quando fizemos
// uma sondagem no começo da inicializaçao do kernel.
// #todo: 
// Podemos salvar essa lista.
// #todo
// O que é PCIDeviceATA?
// É uma estrutura para dispositivos pci. (pci_device_d)
// Vamos mudar de nome.

    PCIDeviceATA = 
        (struct pci_device_d *) scan_pci_device_list2 ( 
                                    (unsigned char) PCI_CLASSCODE_MASS, 
                                    (unsigned char) PCI_SUBCLASS_IDE );

    if ( (void *) PCIDeviceATA == NULL ){
        printf("ata_initialize: PCIDeviceATA\n");
        Status = (int) -1;
        goto fail;
    }

    if ( PCIDeviceATA->used != TRUE || 
         PCIDeviceATA->magic != 1234 )
    {
        printf("ata_initialize: PCIDeviceATA validation\n");
        Status = (int) -1;
        goto fail;
    }

//#debug
    // printf (": IDE device found\n");
    // printf ("[ Vendor=%x Device=%x ]\n", PCIDeviceATA->Vendor, PCIDeviceATA->Device );

// Vamos saber mais sobre o dispositivo encontrado. 
// #bugbug: 
// Esse retorno é só um código de erro.
// Nessa hora configuramos os valores na estrutura 'ata.xxx'

    Value = 
        (unsigned long) atapciConfigurationSpace( (struct pci_device_d*) PCIDeviceATA );

    if ( Value == PCI_MSG_ERROR )
    {
        printk ("ata_initialize: Error Driver [%x]\n", Value );
        Status = (int) -1;
        goto fail;
    }


// Explicando:
// Aqui estamos pegando nas BARs o número das portas.
// Logo em seguida salvaremos esses números e usaremos
// eles para fazer uma rotina de soft reset.
// See:
// https://wiki.osdev.org/PCI_IDE_Controller

// base address 
// BAR0 is the start of the I/O ports used by the primary channel.
// BAR1 is the start of the I/O ports which control the primary channel.
// BAR2 is the start of the I/O ports used by secondary channel.
// BAR3 is the start of the I/O ports which control secondary channel.
// BAR4 is the start of 8 I/O ports controls the primary channel's Bus Master IDE.
// BAR4 + 8 is the Base of 8 I/O ports controls secondary channel's Bus Master IDE.

    ATA_BAR0_PRIMARY_COMMAND_PORT   = ( PCIDeviceATA->BAR0 & ~7 ) + ATA_IDE_BAR0_PRIMARY_COMMAND   * ( !PCIDeviceATA->BAR0 );
    ATA_BAR1_PRIMARY_CONTROL_PORT   = ( PCIDeviceATA->BAR1 & ~3 ) + ATA_IDE_BAR1_PRIMARY_CONTROL   * ( !PCIDeviceATA->BAR1 );       
    ATA_BAR2_SECONDARY_COMMAND_PORT = ( PCIDeviceATA->BAR2 & ~7 ) + ATA_IDE_BAR2_SECONDARY_COMMAND * ( !PCIDeviceATA->BAR2 );
    ATA_BAR3_SECONDARY_CONTROL_PORT = ( PCIDeviceATA->BAR3 & ~3 ) + ATA_IDE_BAR3_SECONDARY_CONTROL * ( !PCIDeviceATA->BAR3 );

    ATA_BAR4 = ( PCIDeviceATA->BAR4 & ~0x7 ) + ATA_IDE_BAR4_BUS_MASTER * ( !PCIDeviceATA->BAR4 );
    ATA_BAR5 = ( PCIDeviceATA->BAR5 & ~0xf ) + ATA_IDE_BAR5            * ( !PCIDeviceATA->BAR5 );

// Colocando nas estruturas.
    ide_ports[0].base_port = (unsigned short) (ATA_BAR0_PRIMARY_COMMAND_PORT   & 0xFFFF);
    ide_ports[1].base_port = (unsigned short) (ATA_BAR1_PRIMARY_CONTROL_PORT   & 0xFFFF);
    ide_ports[2].base_port = (unsigned short) (ATA_BAR2_SECONDARY_COMMAND_PORT & 0xFFFF);
    ide_ports[3].base_port = (unsigned short) (ATA_BAR3_SECONDARY_CONTROL_PORT & 0xFFFF);
    // Tem ainda a porta do dma na bar4.


//
// De acordo com o tipo.
//

// #??
// Em que momento setamos os valores dessa estrutura.
// Precisamos de uma flag que diga que a estrutura ja esta inicializada.
// Caso contrario nao podemos usar os valores dela.
// >> Isso acontece  logo acima quando chamamos
// a funçao atapciConfigurationSpace()

    if ( ata.used != TRUE || 
         ata.magic != 1234 )
    {
        printf("ata_initialize: ata structure validation\n");
        return -1;
    }

// Que tipo de controlador?


// ==============================================
// Se for IDE.

    // Type
    if ( ata.chip_control_type == ATA_IDE_CONTROLLER ){

        //Soft Reset, defina IRQ
        out8(
            (unsigned short) (ATA_BAR1_PRIMARY_CONTROL_PORT & 0xFFFF),
            0xff );
        
        out8( 
            (unsigned short) (ATA_BAR3_SECONDARY_CONTROL_PORT & 0xFFFF), 
            0xff );
        
        out8( 
            (unsigned short) (ATA_BAR1_PRIMARY_CONTROL_PORT & 0xFFFF), 
            0x00 );
        
        out8( 
            (unsigned short) (ATA_BAR3_SECONDARY_CONTROL_PORT & 0xFFFF), 
            0x00 );

        // ??
        ata_record_dev     = 0xff;
        ata_record_channel = 0xff;

//#ifdef KERNEL_VERBOSE
        //printf ("Initializing IDE Mass Storage device ...\n");
        //refresh_screen ();
//#endif    

        //
        // As estruturas de disco serão colocadas em uma lista encadeada.
        
        
        //#deprecated
        //ide_mass_storage_initialize();

        //
        // ready_queue_dev
        //

        // Vamos trabalhar na lista de dispositivos.
        // Iniciando a lista.
        // ata_device_d structure.

        ready_queue_dev = 
            ( struct ata_device_d * ) kmalloc ( sizeof(struct ata_device_d) );

        if ( (void*) ready_queue_dev == NULL ){
            printf("ata_initialize: ready_queue_dev\n");
            Status = (int) -1;
            goto fail;
        }

        //
        // current_sd
        //

        // Initialize the head of the list.
        // Not used.
        // There is a loop to reinitialize the
        // structure of each of all ports.

        current_sd = ( struct ata_device_d * ) ready_queue_dev;
        current_sd->dev_id   = __next_sd_id++;
        current_sd->dev_type = -1;
        // Channel and device.
        // ex: primary/master.
        current_sd->dev_channel = -1;
        current_sd->dev_num     = -1;
        current_sd->dev_nport   = -1;
        current_sd->next = NULL;

        // #bugbug
        // Is this a buffer ? For what ?
        // Is this buffer enough ?

        ata_identify_dev_buf = (unsigned short *) kmalloc(4096);

        if ( (void *) ata_identify_dev_buf == NULL )
        {
            printf ("ata_initialize: ata_identify_dev_buf\n");
            Status = (int) -1;
            goto fail;
        }

        // Sondando dispositivos e imprimindo na tela.
        // As primeiras quatro portas do controlador IDE. 

        // #todo
        // Create a constant for 'max'. 
        
        // We're gonna create the structure for each 
        // of the devices.
        
        for ( 
            iPortNumber=0; 
            iPortNumber < 4; 
            iPortNumber++ )
        {
            ide_dev_init(iPortNumber);
        };

        // Ok
        Status = 0;
        goto done;
    }


// ==============================================
// Agora se for AHCI.

    if ( ata.chip_control_type == ATA_RAID_CONTROLLER )
    {
        printf("ata_initialize: RAID not supported yet\n");
        Status = (int) -1;
        goto fail;
    }

// ==============================================
// Agora se for AHCI.

    if ( ata.chip_control_type == ATA_AHCI_CONTROLLER )
    {
        printf("ata_initialize: AHCI not supported yet\n");
        Status = (int) -1;
        goto fail;
    }


// Controlador de tipo invalido?
    if ( ata.chip_control_type == ATA_UNKNOWN_CONTROLLER )
    {
        printf("ata_initialize: Unknown controller type\n");
        return -1;
    }

// ==============================================
// Nem IDE nem AHCI.

    printf("ata_initialize: IDE and AHCI not found\n");
    Status = (int) -1;

fail:
    printf ("ata_initialize: fail\n");
    return -1;

done:
// Setup interrupt breaker.
// Só liberamos se a inicialização fncionou.
    if ( Status == 0 ){
        debug_print("ata_initialize: Turn off interrupt breaker\n");
        __breaker_ata1_initialized = TRUE; 
        __breaker_ata2_initialized = TRUE; 
    }
    return (int) Status;
}


// ide_identify_device:
// ??
// O número da porta identica qual disco queremos pegar informações.
// Slavaremos algumas informações na estrutura de disco.

// OUT:
// 0xFF = erro ao identificar o número.
// 0    = PATA ou SATA
// 0x80 = ATAPI ou SATAPI

int ide_identify_device ( uint8_t nport )
{
    // Signature bytes.
    unsigned char sig_byte_1=0xFF;
    unsigned char sig_byte_2=0xFF;

    struct disk_d  *disk;
    char name_buffer[32];

    unsigned char status=0;


    debug_print("ide_identify_device: \n");

// #todo
// What is the limit?
// The limit here is 4.
// See: ide.h

    if(nport >= 4){
        panic("ide_identify_device: nport\n");
    }

// #todo:
// Rever esse assert. 
// Precisamos de uma mensagem de erro aqui.

    __ata_config_structure(nport);

// ??
// Ponto flutuante
// Sem unidade conectada ao barramento
// #todo
// Precisamos de uma mensagem aqui.

    if ( ata_status_read() == 0xFF )
    {
        debug_print("ide_identify_device: ata_status_read()\n");
        goto fail;
    }

// #bugbug
// O que é isso?
// Se estamos escrevendo em uma porta de input/output
// então temos que nos certificar que esses valores 
// são válidos.

//Reset?
    out8 ( ata.cmd_block_base_address + ATA_REG_SECCOUNT, 0 );  // Sector Count 7:0
    out8 ( ata.cmd_block_base_address + ATA_REG_LBA0,     0 );  // LBA  7-0
    out8 ( ata.cmd_block_base_address + ATA_REG_LBA1,     0 );  // LBA 15-8
    out8 ( ata.cmd_block_base_address + ATA_REG_LBA2,     0 );  // LBA 23-16


// Select device
// #todo:
// Review the data sent to the port.

    out8( 
        (unsigned short) ( ata.cmd_block_base_address + ATA_REG_DEVSEL), 
        (unsigned char) 0xE0 | ata.dev_num << 4 );

//
// Solicitando informações sobre o disco.
//


    
// cmd
    ata_wait (400);

    debug_print("ide_identify_device: ATA_CMD_IDENTIFY_DEVICE\n");

    ata_cmd_write (ATA_CMD_IDENTIFY_DEVICE); 
    ata_wait (400);

    // ata_wait_irq();

// Nunca espere por um IRQ aqui
// Devido unidades ATAPI, ao menos que pesquisamos pelo Bit ERROR
// Melhor seria fazermos polling
// Sem unidade no canal.



    debug_print("ide_identify_device: ata_status_read()\n");

    if ( ata_status_read() == 0 ){
        debug_print("ide_identify_device: ata_status_read()\n");
        goto fail;
    }

// #todo
// Use esses registradores para pegar mais informações.
// See:
// hal/dev/blkdev/ata.h
// See
// https://wiki.osdev.org/ATA_PIO_Mode
// https://wiki.osdev.org/PCI_IDE_Controller

    /*
    // #exemplo:
    // get the "signature bytes" 
    unsigned cl=inb(ctrl->base + REG_CYL_LO);	
    unsigned ch=inb(ctrl->base + REG_CYL_HI);
    differentiate ATA, ATAPI, SATA and SATAPI 
    if (cl==0x14 && ch==0xEB) return ATADEV_PATAPI;
    if (cl==0x69 && ch==0x96) return ATADEV_SATAPI;
    if (cl==0 && ch == 0) return ATADEV_PATA;
    if (cl==0x3c && ch==0xc3) return ATADEV_SATA;
    */

// Saving.
    // lba1 = in8( ata.cmd_block_base_address + ATA_REG_LBA1 );
    // lba2 = in8( ata.cmd_block_base_address + ATA_REG_LBA2 );

    // REG_CYL_LO = 4
    // REG_CYL_HI = 5

//
// SIGNATURE:
// Getting signature bytes.
//

    sig_byte_1 = in8( ata.cmd_block_base_address + 4 );
    sig_byte_2 = in8( ata.cmd_block_base_address + 5 );



// #todo
// In this moment we're trying to find 
// the size of the disk given in sectors.

/*
//===============

// #test
// Isso provavelmente é o número de setores envolvidos em
// uma operação de leitura ou escrita.

    unsigned long rw_size_in_sectors=0;
    rw_size_in_sectors = in8( ata.cmd_block_base_address + ATA_REG_SECCOUNT );
    if (rw_size_in_sectors>0){
       printf("::#breakpoint NumberOfSectors %d \n",rw_size_in_sectors);
       refresh_screen();
       while(1){}
    }
    //===============
*/

/*

// Sector Number Register 
// (LBAlo) This is CHS / LBA28 / LBA48 specific.
#define ATA_REG_LBA0      0x03

// Cylinder Low Register 
// (LBAmid) Partial Disk Sector address.
#define ATA_REG_LBA1      0x04

// Cylinder High Register 
// (LBAhi) Partial Disk Sector address.
#define ATA_REG_LBA2      0x05

*/

// See:
// https://forum.osdev.org/viewtopic.php?f=1&t=22563

/*
#define DRDY  0x40
#define BSY   0x80
#define HOB   0x80
#define Sector_Count    2
#define LBA_Low         3
#define LBA_Mid         4
#define LBA_High        5
#define Device          6
#define Status          7
#define Command         7

    unsigned long Max_LBA=0;
    unsigned int port = (ata.cmd_block_base_address & 0xFFFF);
*/    

//#atençao
//Isso foi chamado logo acima.
    //out8(port+Command, 0xF8);  //READ NATIVE MAX ADDRESS
    //out8(port+Command, 0xEC);  //ATA_CMD_IDENTIFY_DEVICE


// Wait 
    //while ( in8(port+Status) &  BSY != 0)
    //{
        // wait command completed
    //}

// Wait
    //int w=0;
    //for(w=0;w<400000;w++){}

/*
// Not lba48
    Max_LBA  = (unsigned long long )in8(port+LBA_Low);
    Max_LBA += (unsigned long long )in8(port+LBA_Mid)  <<8;
    Max_LBA += (unsigned long long )in8(port+LBA_High) <<16;
    Max_LBA += ((unsigned long long )in8(port+Device) & 0xF) <<24;
*/

    //unsigned char buffer[512+1];
    //unsigned short buffer[512+1];

    // ata.cmd_block_base_address?
    // Isso foi configurado logo acima,
    // então a função hdd_ata_pio_read
    // vai usar a mesma porta que estamos 
    // usando nessa função.

    //debug_print("ide_identify_device: hdd_ata_pio_read()\n");
  
    
/*
    hdd_ata_pio_read ( 
        (unsigned int) nport, 
        (void *) buffer, 
        (int) 512 );
*/

/*
    ata_wait_drq();
    __ata_pio_read ( buffer, 512 );

    ata_wait_not_busy();
    ata_wait_no_drq();
*/


/*  
    //#todo: 60 se for words e 120 se for bytes.
    Max_LBA = buffer[60] &0xffff;
    Max_LBA += buffer[61] << 16 &0xffffffff;

    if( Max_LBA > 0 )
    {
        printf("::#breakpoint Max_LBA %d \n",Max_LBA);
        //refresh_screen();
        //while(1){}
        
        ide_ports[nport].size_in_sectors = (unsigned long) Max_LBA;
    }
*/

// =============================================================

// #test
// vamos pegar mais informações. 

//
// # Type
//

/*
	if (lba1==0x00 && lba2==0x00) = ATADEV_PATA
	if (lba1==0x3c && lba2==0xc3) = ATADEV_SATA
	if (lba1==0x14 && lba2==0xEB) = ATADEV_PATAPI
	if (lba1==0x69 && lba2==0x96) = ATADEV_SATAPI
*/

// ==========================
// # PATA
    if ( sig_byte_1 == 0 && 
         sig_byte_2 == 0 )
    {
        // kputs("Unidade PATA\n");
        // aqui esperamos pelo DRQ
        // e eviamos 256 word de dados PIO

        ata_wait_drq();
        __ata_pio_read ( ata_identify_dev_buf, 512 );

        ata_wait_not_busy();
        ata_wait_no_drq();

        // See: ide.h

        ide_ports[nport].id = (uint8_t) nport;           // Port index.
        ide_ports[nport].type = (int) idedevicetypesPATA;  // Device type.
        ide_ports[nport].name = "PATA";
        ide_ports[nport].channel = ata.channel;  // Primary or secondary.
        ide_ports[nport].dev_num = ata.dev_num;  // Master or slave.
        ide_ports[nport].used = (int) TRUE;
        ide_ports[nport].magic = (int) 1234;

        // Disk
        // See: disk.h
        disk = (struct disk_d *) kmalloc( sizeof(struct disk_d) );

        if( (void*) disk == NULL){
            debug_print("ide_identify_device: disk on PATA\n");
            goto fail;
        }

        if ((void *) disk != NULL )
        {
            // Object header.
            // #todo
            //disk->objectType = ?
            //disk->objectClass = ?

            disk->used = TRUE;
            disk->magic = 1234;
            // type and class.
            disk->diskType = DISK_TYPE_PATA;
            // disk->diskClass = ? // #todo:

            // ID and index.
            disk->id = nport;
            disk->boot_disk_number = 0; // ?? #todo

            // name
 
            // name = "sd?"
            //disk->name = "PATA-TEST";
            sprintf ( (char *) name_buffer, "PATA-TEST-%d",nport);
            disk->name = (char *) strdup ( (const char *) name_buffer);  

            // Security
            disk->pid = (pid_t) get_current_process(); //current_process;
            disk->gid = current_group;
            // ...

            disk->channel = ata.channel;  // Primary or secondary.
            disk->dev_num = ata.dev_num;  // Master or slave.

            // disk->next = NULL;
            
            // #bugbug
            // #todo: Check overflow.
            diskList[nport] = (unsigned long) disk;
        }

        // It's a PATA device.
        // 0 = PATA or SATA.
        return (int) 0;
    }

// ==========================
// #SATA
    if ( sig_byte_1 == 0x3C && 
         sig_byte_2 == 0xC3 )
    {
        //kputs("Unidade SATA\n");   
        // O dispositivo responde imediatamente um erro ao cmd Identify device
        // entao devemos esperar pelo DRQ ao invez de um BUSY
        // em seguida enviar 256 word de dados PIO.

        ata_wait_drq(); 
        __ata_pio_read ( ata_identify_dev_buf, 512 );
        ata_wait_not_busy();
        ata_wait_no_drq();

        // See: ide.h

        ide_ports[nport].id = (uint8_t) nport;
        ide_ports[nport].type = (int) idedevicetypesSATA;  // Device type.
        ide_ports[nport].name = "SATA";                    // Port name.
        ide_ports[nport].channel = ata.channel;  // Primary or secondary.
        ide_ports[nport].dev_num = ata.dev_num;  // Master or slave.
        ide_ports[nport].used = (int) TRUE;
        ide_ports[nport].magic = (int) 1234;

        // Disk
        // See: disk.h
        disk = (struct disk_d *) kmalloc(  sizeof(struct disk_d) );

        if( (void*) disk == NULL){
            debug_print("ide_identify_device: disk on SATA\n");
            goto fail;
        }

        if ((void *) disk != NULL )
        {

            // Object header.
            // #todo
            //disk->objectType = ?
            //disk->objectClass = ?

            disk->used = TRUE;
            disk->magic = 1234;
            disk->diskType = DISK_TYPE_SATA;
            // disk->diskClass = ?

            // ID and index.
            disk->id = nport;
            disk->boot_disk_number = 0; // ?? #todo

            // name

            //disk->name = "SATA-TEST";
            sprintf ( (char *) name_buffer, "SATA-TEST-%d",nport);
            disk->name = (char *) strdup ( (const char *) name_buffer);  

            // Security
            disk->pid = (pid_t) get_current_process();  //current_process;
            disk->gid = current_group;
              
            disk->channel = ata.channel;  // Primary or secondary.
            disk->dev_num = ata.dev_num;  // Master or slave.
 
            // disk->next = NULL;
            
            // #todo: Check overflow.
            diskList[nport] = (unsigned long) disk;
        }

        // It's a SATA device.
        // 0 = PATA or SATA.

        return (int) 0;
    }


// ==========================
// # PATAPI
    if ( sig_byte_1 == 0x14 && 
         sig_byte_2 == 0xEB )
    {
        //kputs("Unidade PATAPI\n");   
        ata_cmd_write(ATA_CMD_IDENTIFY_PACKET_DEVICE);
        ata_wait(400);
        ata_wait_drq(); 
        __ata_pio_read ( ata_identify_dev_buf, 512 );
        ata_wait_not_busy();
        ata_wait_no_drq();

        // See: ide.h

        ide_ports[nport].id = (uint8_t) nport;
        ide_ports[nport].type = (int) idedevicetypesPATAPI;
        ide_ports[nport].name = "PATAPI";
        ide_ports[nport].channel = ata.channel;  // Primary or secondary.
        ide_ports[nport].dev_num = ata.dev_num;  // Master or slave.
        ide_ports[nport].used = (int) TRUE;
        ide_ports[nport].magic = (int) 1234;

        // Disk
        // See: disk.h

        disk = (struct disk_d *) kmalloc (  sizeof(struct disk_d) );

        if( (void*) disk == NULL){
            debug_print("ide_identify_device: disk on PATAPI\n");
            goto fail;
        }

        if ((void *) disk != NULL )
        {
            disk->used = TRUE;
            disk->magic = 1234;
            disk->diskType = DISK_TYPE_PATAPI;
            // disk->diskClass = ?

            disk->id = nport;  
                        
            // name
            
            //disk->name = "PATAPI-TEST";
            sprintf ( (char *) name_buffer, "PATAPI-TEST-%d",nport);
            disk->name = (char *) strdup ( (const char *) name_buffer);  



            disk->channel = ata.channel;  // Primary or secondary.
            disk->dev_num = ata.dev_num;  // Master or slave.

            // #todo: Check overflow.
            diskList[nport] = (unsigned long) disk;
        }

        // It's a PATAPI device.
        // 0x80 = PATAPI or SATAPI.

        return (int) 0x80;
    }

// ==========================
// # SATAPI
    if (sig_byte_1 == 0x69  && 
        sig_byte_2 == 0x96)
    {
        //kputs("Unidade SATAPI\n");   
        ata_cmd_write(ATA_CMD_IDENTIFY_PACKET_DEVICE);
        ata_wait(400);
        ata_wait_drq(); 
        __ata_pio_read(ata_identify_dev_buf,512);
        ata_wait_not_busy();
        ata_wait_no_drq();

        // See: ide.h

        ide_ports[nport].id = (uint8_t) nport;
        ide_ports[nport].type = (int) idedevicetypesSATAPI;
        ide_ports[nport].name = "SATAPI";
        ide_ports[nport].channel = ata.channel;  // Primary or secondary.
        ide_ports[nport].dev_num = ata.dev_num;  // Master or slave.
        ide_ports[nport].used = (int) TRUE;
        ide_ports[nport].magic = (int) 1234;

        // Disk
        // See: disk.h

        disk = (struct disk_d *) kmalloc(  sizeof(struct disk_d) );

        if( (void*) disk == NULL){
            debug_print("ide_identify_device: disk on SATAPI\n");
            goto fail;
        }

        if ( (void *) disk != NULL )
        {
            disk->used = TRUE;
            disk->magic = 1234;
            disk->diskType = DISK_TYPE_SATAPI;
            //disk->diskClass = ?

            disk->id = nport;  

            // name
            
            //disk->name = "SATAPI-TEST";
            sprintf ( (char *) name_buffer, "SATAPI-TEST-%d",nport);
            disk->name = (char *) strdup ( (const char *) name_buffer);  

            disk->channel = ata.channel;  // Primary or secondary.
            disk->dev_num = ata.dev_num;  // Master or slave.

            // #todo: Check overflow.
            diskList[nport] = (unsigned long) disk;
        }

        // It's a SATAPI device.
        // 0x80 = PATAPI or SATAPI.

        return (int) 0x80;
    }

// fail ??
// Is something wrong here?

    // #debug
    //panic("ide_identify_device: type not defined");


// 0xFF: Unknown device class.
fail:
    debug_print("ide_identify_device: fail\n");
    return 0xFF;
}


// ide_dev_init:
// ?? Alguma rotina de configuração de dispositivos.
// This routine was called by ata_initialize.
// #todo
// Agora essa função precisa receber um ponteiro 
// para a estrutura de disco usada pelo gramado.
// Para salvarmos os valores que pegamos nos registradores.

// Called by ata_initialize.

int ide_dev_init (char port)
{
    struct ata_device_d  *new_dev;

    int isBootTimeIDEPort=FALSE;

    int data=0;

    unsigned long value1=0;
    unsigned long value2=0;
    unsigned long value3=0;
    unsigned long value4=0;


// Limits
// 4 ports only    

    if ( port < 0 || 
         port >= 4)
    {
        printf ("ide_dev_init: port\n");
        goto fail;
    }


// storage
// We need this structure.
    if ( (void*) storage == NULL ){
        panic("ide_dev_init: storage");
    }

// boot disk
// We need this structure.
    if ( (void*) ____boot____disk == NULL ){
        panic("ide_dev_init: ____boot____disk");
    }
    if ( ____boot____disk->magic != 1234 ){
        panic("ide_dev_init: ____boot____disk->magic");
    }

// boot partition
// We need this structure.
    if ( (void*) volume_bootpartition == NULL ){
        panic("ide_dev_init: volume_bootpartition");
    }
    if ( volume_bootpartition->magic != 1234 ){
        panic("ide_dev_init: volume_bootpartition->magic");
    }

// Is it the boottime ide port?

    int boottime_ideport = ata_get_boottime_ide_port_index();
    
    // YES!
    if( port == boottime_ideport)
    {
        isBootTimeIDEPort = TRUE;
    }


//
// new_dev
//

// See: ata.h

    new_dev = ( struct ata_device_d * ) kmalloc( sizeof(struct ata_device_d) );

    if ( (void *) new_dev ==  NULL )
    {
        printf ("ide_dev_init: new_dev\n");
        goto fail;
    }

// ??
// #todo: Explain it.

    data = (int) ide_identify_device(port);

// Erro
// 0xFF = erro ao identificar o número.
// 0    = PATA ou SATA
// 0x80 = ATAPI ou SATAPI


// ================
// Unidade de classe desconhecida.
    if ( data == 0xFF )
    {
        debug_print ("ide_dev_init: [FIXME] data\n");
        return (int) -1;
    }


// ================
// Unidades ATA.
// 0    = PATA ou SATA
    if ( data == 0 ){

        // Is it an ata device?
        new_dev->dev_type = (ata_identify_dev_buf[0] & 0x8000) ? 0xffff : ATA_DEVICE_TYPE;

        // What kind of lba?
        new_dev->dev_access = (ata_identify_dev_buf[83] & 0x0400) ? ATA_LBA48 : ATA_LBA28;

        // Let's set up the PIO support.
        // Where ATAFlag was defined?
        // Where FORCEPIO was defined?
        
        // Com esse só funciona em pio
        if (ATAFlag == FORCEPIO){
            new_dev->dev_modo_transfere = 0;
        // Com esse pode funcionar em dma
        }else{
            new_dev->dev_modo_transfere = 
                 ( ata_identify_dev_buf[49] & 0x0100 ) 
                 ? ATA_DMA_MODO 
                 : ATA_PIO_MODO;
        };

        //old
        //new_dev->dev_total_num_sector  = ata_identify_dev_buf[60];
        //new_dev->dev_total_num_sector += ata_identify_dev_buf[61];

        //#test
        //new_dev->dev_total_num_sector  = 
        //    *(unsigned long*) (short *) &ata_identify_dev_buf[60];


        // #bugbug
        // We can not do this.
        // We need to check the supported sector size.
        // 512 or 4096 ?

        new_dev->dev_byte_per_sector = 512;

        //new_dev->dev_total_num_sector_lba48  = ata_identify_dev_buf[100];
        //new_dev->dev_total_num_sector_lba48 |= ata_identify_dev_buf[101];
        //new_dev->dev_total_num_sector_lba48 |= ata_identify_dev_buf[102];
        //new_dev->dev_total_num_sector_lba48 |= ata_identify_dev_buf[103];

        value1 = ata_identify_dev_buf[103];  
        new_dev->lba48_value1 = (unsigned short) value1;
        value1 = ( value1 << 48 );           
        value1 = ( value1 & 0xFFFF000000000000 );    
        
        value2 = ata_identify_dev_buf[102];  
        new_dev->lba48_value2 = (unsigned short) value2;
        value2 = ( value2 << 32 );
        value2 = ( value2 & 0x0000FFFF00000000  );

        value3 = ata_identify_dev_buf[101];
        new_dev->lba48_value3 = (unsigned short) value3;  
        value3 = ( value3 << 16 );
        value3 = ( value3 & 0x00000000FFFF0000  );

        value4 = ata_identify_dev_buf[100];
        new_dev->lba48_value4 = (unsigned short) value4;  
        //value4 = ( value4 << 0 );
        value4 = ( value4 & 0x000000000000FFFF  );
          
        new_dev->dev_total_num_sector_lba48 = 
            (unsigned long) (value1 | value2 | value3 | value4 );

        
        // #bugbug
        // We can not do this.
        // We need to check the supported sector size.
        // 512 or 4096 ?

        new_dev->dev_size = (new_dev->dev_total_num_sector_lba48 * 512);

        // Pegando o size.
        // Quantidade de setores.
        // Uma parte está em 61 e outra em 60.
        
        value1 = ata_identify_dev_buf[61];  
        new_dev->lba28_value1 = (unsigned short) value1;
        value1 = ( value1 << 16 );           
        value1 = ( value1 & 0xFFFF0000 );    
        value2 = ata_identify_dev_buf[60];
        new_dev->lba28_value2 = (unsigned short) value2;
        value2 = ( value2 & 0x0000FFFF );  
        new_dev->dev_total_num_sector = value1 | value2;
        new_dev->_MaxLBA = new_dev->dev_total_num_sector;
                   
        //if ( new_dev->dev_total_num_sector > 0 )
        //{
        //     printf ("#debug: >>>> ata Size %d\n", 
        //         new_dev->dev_total_num_sector );
        //     refresh_screen();
        //     while(1){}
        //}


// #todo
// Agora essa função precisa receber um ponteiro 
// para a estrutura de disco usada pelo gramado.

// ================
// Unidades ATAPI. 
// 0x80 = ATAPI ou SATAPI
    }else if( data == 0x80 ){

        // Is this an ATAPI device ?
        new_dev->dev_type = (ata_identify_dev_buf[0] & 0x8000) ? ATAPI_DEVICE_TYPE : 0xffff;

        // What kind of lba?
        new_dev->dev_access = ATA_LBA28;

        // Let's set up the PIO support.
        // Where ATAFlag was defined?
        // Where FORCEPIO was defined?

        // Com esse só funciona em pio 
              if (ATAFlag == FORCEPIO){
                  new_dev->dev_modo_transfere = 0; 

              // Com esse pode funcionar em dma
              }else{
                  new_dev->dev_modo_transfere = (ata_identify_dev_buf[49] & 0x0100) ? ATA_DMA_MODO : ATA_PIO_MODO;
              };

              // ??
              new_dev->dev_total_num_sector  = 0;
              new_dev->dev_total_num_sector += 0;



              // #bugbug
              // 2024
              // Is this standard for all kind of CD?
              // We need to get this information in some place.

              new_dev->dev_byte_per_sector = 2048; 

              //#test
              //new_dev->dev_total_num_sector  = 
                  //*(unsigned long*) (short *) &ata_identify_dev_buf[60];


              new_dev->dev_total_num_sector_lba48  = 0;
              new_dev->dev_total_num_sector_lba48 |= 0;
              new_dev->dev_total_num_sector_lba48 |= 0;
              new_dev->dev_total_num_sector_lba48 |= 0;

              value1 = ata_identify_dev_buf[103];  
              new_dev->lba48_value1 = (unsigned short) value1;
              value1 = ( value1 << 48 );           
              value1 = ( value1 & 0xFFFF000000000000 );    
        
              value2 = ata_identify_dev_buf[102];  
              new_dev->lba48_value2 = (unsigned short) value2;
              value2 = ( value2 << 32 );
              value2 = ( value2 & 0x0000FFFF00000000  );

              value3 = ata_identify_dev_buf[101];
              new_dev->lba48_value3 = (unsigned short) value3;  
              value3 = ( value3 << 16 );
              value3 = ( value3 & 0x00000000FFFF0000  );

              value4 = ata_identify_dev_buf[100];
              new_dev->lba48_value4 = (unsigned short) value4;  
              //value4 = ( value4 << 0 );
              value4 = ( value4 & 0x000000000000FFFF  );
          
              new_dev->dev_total_num_sector_lba48 = 
                  (unsigned long) (value1 | value2 | value3 | value4 );

            
              // #bugbug
              // 2024
              // Is this standard for all kind of CD?
              // We need to get this information in some place.

              new_dev->dev_size = (new_dev->dev_total_num_sector_lba48 * 2048);

              // Pegando o size.
              // Quantidade de setores.
              // Uma parte está em 61 e outra em 60.
 
              value1 = ata_identify_dev_buf[61];
              new_dev->lba28_value1 = (unsigned short) value1;  
              value1 = ( value1 << 16 );           
              value1 = ( value1 & 0xFFFF0000 );    
              value2 = ata_identify_dev_buf[60];
              new_dev->lba28_value2 = (unsigned short) value2;  
              value2 = ( value2 & 0x0000FFFF );  
              new_dev->dev_total_num_sector = value1 | value2;
              new_dev->_MaxLBA = new_dev->dev_total_num_sector;
              
              
              //if ( new_dev->dev_total_num_sector > 0 )
              //{
              //   printf ("#debug: >>>> atapi Size %d\n", 
              //       new_dev->dev_total_num_sector );
              //   refresh_screen();
              //   while(1){}
              //}

             // #todo
             // Agora essa função precisa receber um ponteiro 
             // para a estrutura de disco usada pelo gramado.


// ================
// Unidade de classe desconhecida.
// #bugbug: Not panic()
    }else{
        debug_print ("ide_dev_init: [ERROR] not ATA, not ATAPI.\n");
        return (int) -1;
    };


//
// Dados em comum.
//

    new_dev->dev_id = __next_sd_id++;

// Salvando na estrutura de dispositivo as
// informações sobre a porta ide.
// channel and device.
// ex: primary/master.
// #bugbug
// Mas temos um problema. Talvez quando essa função
// foi chamada o dev_num ainda não tenha cido inicializado.

    new_dev->dev_channel = ata.channel;
    new_dev->dev_num     = ata.dev_num;

    new_dev->dev_nport = port;


//
// bootitme device?
//

// This is the boottime ide port.
// So we're gonna save the pointer
// into the boot disk structure.

// Not a boottime device.
    new_dev->boottime_device = FALSE;

    if (isBootTimeIDEPort == TRUE )
    {
        if ( (void*) ____boot____disk != NULL )
        {
            if ( ____boot____disk->magic == 1234 )
            {
                // ??
                // Only for ATA devices.
                ____boot____disk->ata_device = 
                    (struct ata_device_d *) new_dev;
                
                new_dev->disk = (struct disk_d *) ____boot____disk;
                
                // YES, it's a boottime device.
                new_dev->boottime_device = TRUE;
            }
        }
    }


//
// == port ====================================
//

// #bugbug
// Nao devemos confundir esses numeros com os numeros
// gerados pelo BIOS, pois bios tambem considera
// outras midias alem do ide.

// #atenção
// Essa estrutura é para 32 portas.
// para listar as portas AHCI.
// Mas aqui está apenas listando as 4 portas IDE.

    switch (port){
    case 0:  dev_nport.dev0 = 0x81;  break;
    case 1:  dev_nport.dev1 = 0x82;  break;
    case 2:  dev_nport.dev2 = 0x83;  break;
    case 3:  dev_nport.dev3 = 0x84;  break;
    
    default:
        // #bugbug
        // Porque não retornamos daqui mesmo?
        debug_print ("ide_dev_init: [ERROR] default port number\n");
        break;
    };

// Linked list.
    new_dev->next = NULL;


//#ifdef KERNEL_VERBOSE
    // #todo
    // kprintf("[ Detected Disk type: %s ]\n", dev_type[new_dev->dev_type] );
    // refresh_screen ();
//#endif


// =========================================

//
// Add no fim da lista (ready_queue_dev).
//
    struct ata_device_d  *tmp;

    tmp = ( struct ata_device_d * ) ready_queue_dev;
    if ( (void *) tmp ==  NULL )
    {
        printf ("ide_dev_init: [FAIL] tmp\n");
        goto fail;
    }

// Linked list. Walk.
// Colocaremos esse novo ponteiro no fim da lista.

    while ( tmp->next )
    {
        tmp = tmp->next;
    };

    tmp->next = new_dev;

//done:
    //debug_print ("ide_dev_init: done\n");
    return 0;

fail:
    refresh_screen();
    panic ("ide_dev_init: fail\n");
    return -1;  // Not reached.
}


void ata_show_device_list_info(void)
{
    struct ata_device_d *sd;

    unsigned long mb28=0;
    unsigned long mb48=0;

// The head of the list
    sd = (struct ata_device_d *) ready_queue_dev;
    
    while ( (void *) sd != NULL ){

    if(sd->boottime_device == TRUE){
        printf("\n");
        printf("The boot device is the port %d\n",sd->dev_nport);
    }

    printf("PORT %d: lba28{%d} lba48{%d}\n",
        sd->dev_nport, 
        sd->dev_total_num_sector,
        sd->dev_total_num_sector_lba48 );

    //#debug
    //printf("PORT %d: LBA28 v1{%d} v2{%d} \n",
        //sd->dev_nport, 
        //sd->lba28_value1,
        //sd->lba28_value2 );

    //#debug
    //printf("PORT %d: LBA48 v1{%d} v2{%d} v3{%d} v4{%d}\n",
        //sd->dev_nport, 
        //sd->lba48_value1,
        //sd->lba48_value2,
        //sd->lba48_value3,
        //sd->lba48_value4 );

    mb28 = (unsigned long) (((sd->dev_total_num_sector * 512)/1024)/1024);
    mb48 = (unsigned long) (((sd->dev_total_num_sector_lba48 * 512)/1024)/1024);

    printf("LBA28 {%d MB} LBA48{%d MB}\n",
        mb28, mb48 );

    sd = (struct ata_device_d *) sd->next;

    };
}


/* 
 * dev_switch:
 *     ?? Porque esse tipo ?? 
 */

static inline void dev_switch (void)
{

// ??
// Pula, se ainda não tiver nenhuma unidade.

    if ( !current_sd )
    {
        return;
    }

// Obter a próxima tarefa a ser executada.
// Se caímos no final da lista vinculada, 
// comece novamente do início.

    current_sd = current_sd->next;    
    
    if ( !current_sd )
    {
        current_sd = ready_queue_dev;
    }
}


static inline int getnport_dev (void)
{
    if ( (void *) current_sd == NULL )
    {
        return -1;
    }

    return (int) current_sd->dev_nport;
}


// #todo
// Explain it better.
int nport_ajust ( char nport )
{
    char i = 0;

    // #todo
    // Simplify this thing.
 
    while ( nport != getnport_dev() )
    {
        if ( i == 4 )
        { 
            return (int) 1; 
        }
        
        dev_switch ();
        
        i++;
    };

    if ( getnport_dev() == -1 )
    { 
        return (int) 1; 
    }

    return 0;
}


/*
 * show_ide_info:
 *     Mostrar as informações obtidas na inicializações 
 * do controlador.
 */

// #todo
// Not used. Please, call this routine.

void show_ide_info (void)
{
    int i=0;

    printf("\n");
    printf ("\n  show_ide_info:  \n");

    for ( i=0; i<4; i++ )
    {
        printf("\n");
        printf ("id=%d \n", ide_ports[i].id );
        printf ("channel=%d dev_num=%d \n", 
            ide_ports[i].channel, 
            ide_ports[i].dev_num );
        //printk ("used=%d magic=%d \n", 
        //    ide_ports[i].used, 
        //    ide_ports[i].magic );
        printf ("type=%d      \n", ide_ports[i].type );
        printf ("base_port=%x \n", ide_ports[i].base_port );
        printf ("name=%s      \n", ide_ports[i].name );
        
        printf ("Size in sectors = %d \n", 
            ide_ports[i].size_in_sectors );
    };

//
// # debug.
//

// primary secondary  ... master slave
    // printf ( " channel=%d dev=%d \n", ata.channel, ata.dev_num );


	/*
	// Estrutura 'ata'
	// Qual lista ??
	
	//pegar a estrutura de uma lista.
	
	//if( ata != NULL )
	//{
		printf("ata:\n");
 	    printf("type={%d}\n", (int) ata.chip_control_type);
	    printf("channel={%d}\n", (int) ata.channel);
	    printf("devType={%d}\n", (int) ata.dev_type);
	    printf("devNum={%d}\n", (int) ata.dev_num);
	    printf("accessType={%d}\n", (int) ata.access_type);
	    printf("cmdReadMode={%d}\n", (int) ata.cmd_read_modo);
	    printf("cmdBlockBaseAddress={%d}\n", (int) ata.cmd_block_base_address);
	    printf("controlBlockBaseAddress={%d}\n", (int) ata.ctrl_block_base_address);
		printf("busMasterBaseAddress={%d}\n", (int) ata.bus_master_base_address);
		printf("ahciBaseAddress={%d}\n", (int) ata.ahci_base_address);
	//};
	*/


	// Estrutura 'atapi'
	// Qual lista ??

	// Estrutura 'ata_device_d'
	// Estão na lista 'ready_queue_dev'	

    //refresh_screen ();
}

