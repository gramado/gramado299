
// e1000.c
// e1000 Intel nic driver.
// jul 2022.
// Ported from gramado 32bit.

#include <kernel.h>


// How many buffers.
// #define E1000_NUM_TX_DESC 8
// #define E1000_NUM_RX_DESC 32
#define SEND_BUFFER_MAX       8
#define RECEIVE_BUFFER_MAX   32


// little endian
#define ToNetByteOrder16(v)   ((v >> 8) | (v << 8))
#define ToNetByteOrder32(v)   (((v >> 24) & 0xFF) | ((v << 8) & 0xFF0000) | ((v >> 8) & 0xFF00) | ((v << 24) & 0xFF000000))
#define FromNetByteOrder16(v) ((v >> 8) | (v << 8))
#define FromNetByteOrder32(v) (((v >> 24) & 0xFF) | ((v << 8) & 0xFF0000) | ((v >> 8) & 0xFF00) | ((v << 24) & 0xFF000000))

//extra
#define e1000_FromNetByteOrder16(v) ((v >> 8) | (v << 8))



//
// == Ethernet ==============================================
//

// Ethernet header length
#define ETHERNET_HEADER_LENGHT  14  

// Ethernet header
struct e1000_ether_header_d 
{
// MAC
    uint8_t dst[6];
    uint8_t src[6];
// Protocol type
    uint16_t type;

} __attribute__((packed)); 


//see: nicintel.h
struct intel_nic_info_d  *currentNIC;



int e1000_interrupt_flag=0;
int e1000_irq_count=0;


unsigned long gE1000InputTime=0;


// NIC device handler.
static void DeviceInterface_e1000(void);

// handle package.
static void 
on_receiving ( 
    const unsigned char *buffer, 
    ssize_t size );

static uint32_t 
__E1000ReadCommand ( 
    struct intel_nic_info_d *d, 
    uint16_t addr );

static void 
__E1000WriteCommand ( 
    struct intel_nic_info_d *d, 
    uint16_t addr, 
    uint32_t val );

static uint32_t 
__E1000ReadEEPROM ( 
    struct intel_nic_info_d *d, 
    uint8_t addr );

static unsigned long 
__E1000AllocCont ( 
    uint32_t amount, 
    unsigned long *virt );  //#todo: 64bit address

static unsigned long __mapping_nic1_device_address(unsigned long pa);

static void
__e1000_enable_interrupt(struct intel_nic_info_d *nic_info);


static int __e1000_reset_controller(void);
static void __e1000_setup_irq (int irq_line);

//
// =====================
//

static uint32_t 
__E1000ReadCommand ( 
    struct intel_nic_info_d *d, 
    uint16_t addr )
{

// Read from memory mapped register.
    return *( (volatile unsigned int *) (d->mem_base + addr));
}

static void 
__E1000WriteCommand ( 
    struct intel_nic_info_d *d, 
    uint16_t addr, 
    uint32_t val )
{

// Write to memory mapped register.
    *( (volatile unsigned int *)(d->mem_base + addr)) = val;
}


static uint32_t 
__E1000ReadEEPROM ( 
    struct intel_nic_info_d *d, 
    uint8_t addr )
{
    uint32_t data=0;


    // #todo
    // Check the pointer validation.

    //if ( (void*) d == NULL )
        //return 0;

	// We have the EEPROM?
 
	//#debug
	//printf("E1000ReadEEPROM:\n");

    // Yes :)	
    if (d->eeprom == 1) {
        __E1000WriteCommand ( d, 0x14, 1 | (addr << 8) );

        //#bugbug
		//#obs: loop		
        while (( (data = __E1000ReadCommand ( d, 0x14)) & 0x10 ) != 0x10 );

	// Nope ...
    } else {
        __E1000WriteCommand ( d, 0x14, 1 | (addr << 2) );

		//#bugbug
		//#obs: loop
        while (( (data = __E1000ReadCommand(d, 0x14)) & 0x01 ) != 0x01 );
    };

    return (data >> 16) & 0xFFFF; 
}


/*
 * E1000AllocCont: ??
 *     Retorna o endereço físico e 
 * coloca o virtual em *virt
 * ah ... então eu vou alocar usando endereços virtuais
 * ... e traduzir para físico 
 * ... colocar o virtual em *virt e retornar o físico.
 */
// Precisamos de um endereço fisico.
// + alocamos um endereço virtual
// + convertemos para fisico
// IN:  size, return virtual address.
// OUT: physical address

static unsigned long 
__E1000AllocCont ( 
    uint32_t amount, 
    unsigned long *virt )  //#todo: 64bit address
{
    unsigned long va=0;
    unsigned long pa=0;

    if (amount==0){
        panic ("__E1000AllocCont: [FAIL] amount\n");
    }

// ============
// va
    va = (unsigned long) kmalloc ( (size_t) amount );
    *virt = va;
    if (*virt == 0){
        panic ("__E1000AllocCont: [FAIL] va allocation\n");
    }
    //printf("va=%x\n",va);


// ============
// pa
// ps: Using the kernel page directory.
// see: 

    pa = 
        (unsigned long) virtual_to_physical (
                            va, 
                            gKernelPML4Address ); 
    if (pa == 0){
        panic ("__E1000AllocCont: [FAIL] pa\n");
    }
    //printf("pa=%x\n",pa);
    
    return (unsigned long) pa;
}



static void
__e1000_enable_interrupt(struct intel_nic_info_d *nic_info)
{
    if( (void*) nic_info == NULL ){
        panic("__e1000_enable_interrupt: nic_info\n");
    }

// 0xD0 Message Control (0x0080) Next Pointer (0xE0) Capability ID (0x05)
//    0000 0001  1111 0111  0000   0010  1101   0111
// 0x1F6DC, 1f72d7

// enable interrupts
    __E1000WriteCommand (nic_info, 0xD0, 0x1F6DC);
    //__E1000WriteCommand(nic_info, 0xD0, 0xFB);
    __E1000WriteCommand(nic_info, 0xD0, 0xff & ~4);  
    __E1000ReadCommand (nic_info, 0xC0);

}

// Reset the controller.
static int __e1000_reset_controller(void)
{
    int i=0;
    //register int i=0;

    debug_print ("__e1000_reset_controller:\n");
    printf      ("__e1000_reset_controller:\n");
    
	//unsigned long tmp;
	
	//#debug
	//printf("__e1000_reset_controller: Reseting controller ... \n");
	
	/*
	if ( (void *) currentNIC ==  NULL )
	{
		printf("__e1000_reset_controller: currentNIC struct\n");
	    return (int) 1;
	}
    */

	//#todo: precisamos checar a validade dessa estrutura e do endereço.
	
    //esse será o endereço oficial.
    //currentNIC->mem_base

    if ( currentNIC->mem_base == 0 ){
        panic ("__e1000_reset_controller: [FAIL] currentNIC->mem_base\n");
    }


	//endereço base.
	//unsigned char *base_address = (unsigned char *) currentNIC->registers_base_address;
	//unsigned long *base_address32 = (unsigned long *) currentNIC->registers_base_address;	

	//unsigned char *base_address = (unsigned char *) currentNIC->mem_base;
	//unsigned long *base_address32 = (unsigned long *) currentNIC->mem_base;	
		
	//
	//===========================================
	//
	
//
//    ## TX ##
//
    //printf("[1]:\n");

    // And alloc the phys/virt address of the transmit buffer
    // tx_descs_phys conterá o endereço físico e
    // legacy_tx_descs conterá o endereço virtual.

    // IN:  size, return virtual address.
    // OUT: physical address

    unsigned long tx_address = (unsigned long) &currentNIC->legacy_tx_descs;

    //printf("tx_address=%x\n",tx_address);
        
    currentNIC->tx_descs_phys = 
        (unsigned long) __E1000AllocCont ( 0x1000, (unsigned long *) tx_address );

    if ( currentNIC->tx_descs_phys == 0 ){
        panic ("__e1000_reset_controller: [FAIL] currentNIC->tx_descs_phys\n");
    }

    //printf("[2]:\n");

// tx

    for ( i=0; i < 8; i++ ) 
    {
        // Alloc the phys/virt address of this transmit desc
        // alocamos memória para o buffer, 
        // salvamos o endereço físico do buffer e 
        // obtemos o endereço virtual do buffer.		

        // IN:  size, return virtual address.
        // OUT: physical address
        
        unsigned long txaddress = 
            (unsigned long) __E1000AllocCont ( 
                 0x3000, 
                 (unsigned long *) &currentNIC->tx_descs_virt[i] );
        
        currentNIC->legacy_tx_descs[i].addr  = (unsigned int) txaddress;
        currentNIC->legacy_tx_descs[i].addr2 = (unsigned int) (txaddress>>32);

        if (currentNIC->legacy_tx_descs[i].addr == 0){
            panic ("__e1000_reset_controller: [FAIL] dev->rx_descs[i].addr\n");
        }

        // #test: Configurando o tamanho do buffer
        currentNIC->legacy_tx_descs[i].length = 0x3000;

        //cmd: bits
        //IDE VLE DEXT RSV RS IC IFCS EOP
        //IDE (bit 7) - Interrupt Delay Enable
        //VLE (bit 6) - VLAN Packet Enable
        //DEXT (bit 5) - Descriptor extension (#importante: '0' for legacy mode)
        //RSV (bit 4) - Reserved
        //RS (bit 3) - Report status
        //IC (bit 2) - Insert checksum
        //IFCS (bit 1) - Insert FCS (CRC)
        //EOP (bit 0) - End of packet

        currentNIC->legacy_tx_descs[i].cmd = 0;
        currentNIC->legacy_tx_descs[i].status = 1;
    };

//#debug 
//Vamos imprimir os endereços usados pelos buffers para teste.	
    //for ( i=0; i < 8; i++ )
    //    printf ("PA={%x} VA={%x} \n",currentNIC->legacy_tx_descs[i].addr, currentNIC->tx_descs_virt[i]);

//
//=============================================
//

//
//    ## RX ##
//

// And alloc the phys/virt address of the transmit buffer

    //printf("[3]:\n");

    currentNIC->rx_descs_phys = 
        __E1000AllocCont (
            0x1000, 
            (unsigned long *)(&currentNIC->legacy_rx_descs));

    if (currentNIC->rx_descs_phys == 0){
        panic ("__e1000_reset_controller: [FAIL] currentNIC->rx_descs_phys\n");
    }

    //printf("[4]:\n");

// rx

    for ( i=0; i < 32; i++ ) 
    {
        // Alloc the phys/virt address of this transmit desc
        // IN:  size, return virtual address.
        // OUT: physical address

        unsigned long rxaddress = 
            (unsigned long) __E1000AllocCont ( 
                0x3000, 
                (unsigned long *) &currentNIC->rx_descs_virt[i] );
        
        currentNIC->legacy_rx_descs[i].addr  = (unsigned int) rxaddress;
        currentNIC->legacy_rx_descs[i].addr2 = (unsigned int) (rxaddress>>32);

        // Buffer null.
        if (currentNIC->legacy_rx_descs[i].addr == 0){
            panic ("__e1000_reset_controller: [FAIL] dev->rx_descs[i].addr\n");
        }

        // #test: Configurando o tamanho do buffer
        currentNIC->legacy_rx_descs[i].length = 0x3000;
        currentNIC->legacy_rx_descs[i].status = 0;
    };

//#debug 
//Vamos imprimir os endereços edereços físicos dos buffers 
//e os edereços virtuais dos descritores.
    //for ( i=0; i < 32; i++ )
    //    printf ("PA={%x} VA={%x} \n",currentNIC->legacy_rx_descs[i].addr, currentNIC->rx_descs_virt[i]);

// ???

    //printf("[5]:\n");

// Clear Multicast Table Array (MTA).
    for (i=0; i < 128; i++){
        __E1000WriteCommand ( currentNIC, 0x5200 + (i * 4), 0 );
    };

// #todo 
// Initialize statistics registers.
// E1000_REG_CRCERRS
    //for (i = 0; i < 64; i++)
    //    __E1000WriteCommand(currentNIC, 0x04000 + i * 4, 0);

// Current.
    currentNIC->rx_cur = 0;
    currentNIC->tx_cur = 0;

//irq #todo
//#bugbug: fow now we're doing this in pci.c.
    //PCIRegisterIRQHandler ( bus, dev, fun, (unsigned long) E1000Handler, currentNIC );

    /* Transmit Enable. */
    //#define E1000_REG_TCTL_EN	(1 << 1)
    /* Pad Short Packets. */
    //#define E1000_REG_TCTL_PSP	(1 << 3)

   //#define E1000_ICR      0x000C0  /* Interrupt Cause Read - R/clr */
   //#define E1000_ITR      0x000C4  /* Interrupt Throttling Rate - RW */
   //#define E1000_ICS      0x000C8  /* Interrupt Cause Set - WO */
   //#define E1000_IMS      0x000D0  /* Interrupt Mask Set - RW */
   //#define E1000_IMC      0x000D8  /* Interrupt Mask Clear - WO */
   //#define E1000_IAM      0x000E0  /* Interrupt Acknowledge Auto Mask */	

   // #define E1000_IAC      0x04100  /* Interrupt Assertion Count */
   //#define E1000_ICRXPTC  0x04104  /* Interrupt Cause Rx Packet Timer Expire Count */
   //#define E1000_ICRXATC  0x04108  /* Interrupt Cause Rx Absolute Timer Expire Count */
   //#define E1000_ICTXPTC  0x0410C  /* Interrupt Cause Tx Packet Timer Expire Count */
   //#define E1000_ICTXATC  0x04110  /* Interrupt Cause Tx Absolute Timer Expire Count */
   //#define E1000_ICTXQEC  0x04118  /* Interrupt Cause Tx Queue Empty Count */
   //#define E1000_ICTXQMTC 0x0411C  /* Interrupt Cause Tx Queue Minimum Threshold Count */
   //#define E1000_ICRXDMTC 0x04120  /* Interrupt Cause Rx Descriptor Minimum Threshold Count */
   //#define E1000_ICRXOC   0x04124  /* Interrupt Cause Receiver Overrun Count */  


   // (*((uint32_t *) (start + E1000_IMS))) |= E1000_IMS_RXT0;
   //(*((uint32_t *) (start + E1000_IMS))) |= E1000_IMS_RXO;
   // (*((uint32_t *) (start + E1000_IMS))) |= E1000_IMS_RXDMT0;
   // (*((uint32_t *) (start + E1000_IMS))) |= E1000_IMS_RXSEQ;
   // (*((uint32_t *) (start + E1000_IMS))) |= E1000_IMS_LSC;	

	//E1000WriteCommand(currentNIC, 0xD0, E1000_IMS_RXT0 | E1000_IMS_RXO );

    //printf("[6]:\n");

// Enable interrupts. (first time)
     __e1000_enable_interrupt(currentNIC);


// ## RX ##
// receive
// Setup the (receive) ring registers.
// Pass the physical address (and some other informations) of the receive buffer

// Address: low and high.
    __E1000WriteCommand (
        currentNIC, 
        0x2800, 
        (unsigned int) currentNIC->rx_descs_phys );  // low
    __E1000WriteCommand (
        currentNIC, 
        0x2804, 
        (unsigned int) (currentNIC->rx_descs_phys >> 32) );                           // high 

// Buffer
    __E1000WriteCommand (currentNIC, 0x2808, 512);    // 32*16
    __E1000WriteCommand (currentNIC, 0x2810, 0);      // head
    __E1000WriteCommand (currentNIC, 0x2818, 31);     // tail

// receive control
// RCTL = 0x0100, /* Receive Control */
    __E1000WriteCommand (
        currentNIC, 
        0x100, 
        0x602801E );  

// RX Delay Timer Register
    //__E1000WriteCommand (currentNIC, 0x2820, 0);

// ## TX ##
//transmit
//Setup the (transmit) ring registers.
// Pass the physical address (and some other informations) of the transmit buffer
//TDBAL	= 0x3800,	/* Tx Descriptor Base Address Low */
//TDBAH	= 0x3804,	/* Tx Descriptor Base Address High */

    
    __E1000WriteCommand (
        currentNIC, 
        0x3800, 
        (unsigned int) currentNIC->tx_descs_phys );  //low (endereço do ring)
    __E1000WriteCommand (
        currentNIC, 
        0x3804, 
        (unsigned int) (currentNIC->tx_descs_phys >> 32) );                           //high

// Buffer
    __E1000WriteCommand (currentNIC, 0x3808, 128);  //8*16
    __E1000WriteCommand (currentNIC, 0x3810, 0);    //head
    __E1000WriteCommand (currentNIC, 0x3818, 7);    //0);      //tail

	//#define E1000_TCTL     0x00400  /* TX Control - RW */
    //• CT = 0x0F (16d collision)
    //• COLD: HDX = 511 (0x1FF); FDX = 63 (0x03F)
    //• PSP = 1b
    //• EN=1b
    //• All other fields 0b	
     /* Transmit Control */
    //#define E1000_TCTL_RST    0x00000001    /* software reset */
    //#define E1000_TCTL_EN     0x00000002    /* enable tx */
    //#define E1000_TCTL_BCE    0x00000004    /* busy check enable */
    //#define E1000_TCTL_PSP    0x00000008    /* pad short packets */
    //#define E1000_TCTL_CT     0x00000ff0    /* collision threshold */
    //#define E1000_TCTL_COLD   0x003ff000    /* collision distance */
    //#define E1000_TCTL_SWXOFF 0x00400000    /* SW Xoff transmission */
    //#define E1000_TCTL_PBE    0x00800000    /* Packet Burst Enable */
    //#define E1000_TCTL_RTLC   0x01000000    /* Re-transmit on late collision */
    //#define E1000_TCTL_NRTU   0x02000000    /* No Re-transmit on underrun */
    //#define E1000_TCTL_MULR   0x10000000    /* Multiple request support */

	//habilita esses dois campos e o resto é zero.
    //• GRAN = 1b (descriptors)
    //• WTHRESH = 1b
    //• All other fields 0b.	
	//#define E1000_TXDCTL_WTHRESH 0x003F0000 /* TXDCTL Writeback Threshold */
	//#define E1000_TXDCTL_GRAN    0x01000000 /* TXDCTL Granularity */
	//#define E1000_TXDCTL   0x03828  /* TX Descriptor Control - RW */

    __E1000WriteCommand (
        currentNIC, 
        0x3828, 
        (0x01000000 | 0x003F0000) );

    __E1000WriteCommand ( 
        currentNIC, 
        0x400, 
        ( 0x00000ff0 | 0x003ff000 | 0x00000008 | 0x00000002) ); 

    //?
	//E1000WriteCommand(currentNIC, 0x400, 0x10400FA);  //TCTL	= 0x0400,	/* Transmit Control */
	//E1000WriteCommand(currentNIC, 0x400, 0x3003F0FA);
	//E1000WriteCommand(currentNIC, 0x400, (1 << 1) | (1 << 3) );
 
    //• IPGT = 8
    //• IPGR1 = 2
    //• IPGR2 = 10
    //#define E1000_TIPG     0x00410  /* TX Inter-packet gap -RW */	
    
    __E1000WriteCommand ( 
        currentNIC,
        0x410,
        (  0x0000000A | 0x00000008 | 0x00000002) ); 

// Talvez ja fizemos isso. 
// Initialize the transmit descriptor registers (TDBAL, TDBAH, TDL, TDH, and TDT).

    //eth_write(base_addr, REG_ADDR_MAC_CONF,
	//	  /* Set the RMII speed to 100Mbps */
	//	  MAC_CONF_14_RMII_100M |
	//	  /* Enable full-duplex mode */
	//	  MAC_CONF_11_DUPLEX |
	//	  /* Enable transmitter */
	//	  MAC_CONF_3_TX_EN |
	//	  /* Enable receiver */
	//	  MAC_CONF_2_RX_EN);	
    
	
	//iow32(dev, TCTL, TCTL_EN);
	
	//printf("nic_i8254x_reset: Done\n");
	//refresh_screen();	
	
	//endereço físico  dos rings;
	//printf("tx_ring_pa=%x rx_ring_pa=%x \n", 
	//    currentNIC->rx_descs_phys, 
	//	currentNIC->tx_descs_phys );

// Enable interrupts. (second time)
     __e1000_enable_interrupt(currentNIC);

// Linkup
// (1 << 6)
// #todo: Create a helper. 
    unsigned int val = __E1000ReadCommand (currentNIC, 0);
    __E1000WriteCommand (currentNIC, 0, val | 0x40);

    return 0;
}


//=======================================================
// __e1000_setup_irq:
//     Setup nic irq 
// #importante
// Isso é usado por uma hotina em headlib.s para 
// configurar uma nova entrada na idt. 
// Essa função é chamada pelo driver de PCI quando encontrar
// o dispositivo Intel apropriado.
// #bugbug
// Called by pciHandleDevice.

//o assembly tem que pegar aqui.
uint8_t nic_idt_entry_new_number;
//na verdade o assembly esta usando outro endereço
//uint32_t nic_idt_entry_new_address;
unsigned long nic_idt_entry_new_address=0; //global

static void __e1000_setup_irq(int irq_line)
{
    debug_print ("__e1000_setup_irq: [FIXME]\n");

// irq line.
    uint8_t irq = (uint8_t) (irq_line & 0xFF);

// handler address.
// #bugbug: 
// Na verdade o assembly esta usando outro endereço.
// #todo: 64bit.
    unsigned long handler = (unsigned long) &irq_E1000; 

// #importante
// Transformando irq em número de interrupção.
// 9+32=41.
    uint8_t idt_num = (uint8_t) (irq + 32);

// --------------

//#debug OK (irq=9) 
    printf ("__e1000_setup_irq: irq={%d}\n", irq);
    printf ("__e1000_setup_irq: idt_num={%d}\n", idt_num);
    printf ("__e1000_setup_irq: handler={%x}\n", handler);
	//printf ("PCIRegisterIRQHandler: pin={%d}\n",currentNIC->pci->irq_pin);//shared INTA#
	//refresh_screen();
	//while(1){}
	//#debug interrupção=41 
     
	//refresh_screen();
	//while(1){}

//
// Creating IDT entry.
//

// Chamando asm:
// número e endereço.
// #obs: Essas variáveis são declaradas nesse arquivo
// o assembly terá que pegar.
// Essa é a rotina em assembly que cria uma entrada na idt para 
// o nic, com base nas variáveis que são importadas pelo assembly.
// headlib.asm
// deveria ir para hwlib.asm

    nic_idt_entry_new_number = 
        (unsigned long) (idt_num & 0xFF);

//not used for now.
//na verdade o assembly esta usando outro endereço
    nic_idt_entry_new_address = (unsigned long) handler; 

    printf ("__e1000_setup_irq: interrupt={%d}\n", 
        nic_idt_entry_new_number );
    printf ("__e1000_setup_irq: handler={%x}\n", 
        nic_idt_entry_new_address );

// Call assembly.
    extern void asm_nic_create_new_idt_entry(void);
    asm_nic_create_new_idt_entry();
}


static unsigned long __mapping_nic1_device_address(unsigned long pa)
{
    // 0x00088000
    unsigned long *nic1_page_table = (unsigned long *) PAGETABLE_NIC1;
    unsigned long nic1_pa = (unsigned long) pa;
    //unsigned long flags = (unsigned long) ( PAGE_WRITE | PAGE_PRESENT );  // flags=3

// 10=cache desable 8= Write-Through 0x002 = Writeable 0x001 = Present
// 0001 1011
    unsigned long flags = (unsigned long) 0x1B;

    mm_fill_page_table( 
        (unsigned long) KERNEL_PD_PA,          // pd 
        (int) PD_ENTRY_NIC1,                   // entry
        (unsigned long) &nic1_page_table[0],   // pt
        (unsigned long) pa,                    // region base
        (unsigned long) flags );               // flags=1b

//entry=393
//see: gentry.h
    unsigned long nic1_va = 0x0000000031200000;

    return (unsigned long) nic1_va; 
}


// ---------------------------------------------
// e1000_init_nic:
// Called by ...
// Initialize the driver.
// == NIC Intel. ===================
// #bugbug
// Ver em que hora que os buffers são configurados.
// precisam ser os mesmos encontrados na 
// infraestrutura de network e usados pelos aplicativos.
// #todo
// O driver funciona na virtualbox,
// se optarmos por PIIX3. Em ICH9 não funciona.
// Estamos suspendendo porque as interrupçoes
// geram muito ruido e a inicialização nem consegue
// terminar. Talvez tenha algo a ver com habilitar
// as interrupções antes do momento em que o
// init habilita as interrupções.

int 
e1000_init_nic ( 
    unsigned char bus, 
    unsigned char dev, 
    unsigned char fun, 
    struct pci_device_d *pci_device )
{
    register uint32_t i=0;  // loop
    uint32_t data=0;        // pci info 
    unsigned short Vendor=0;
    unsigned short Device=0;
    unsigned long phy_address=0;
    unsigned long virt_address=0;
    unsigned short tmp16=0;
    uint32_t Val=0;

// #debug
    debug_print ("e1000_init_nic:\n");
    printf      ("e1000_init_nic:\n");

    //printf("b=%d d=%d f=%d \n", D->bus, D->dev, D->func );
    //printf("82540EM Gigabit Ethernet Controller found\n");

// NIC Intel.
// #importante
// Devemos falhar antes de alocarmos memória para a estrutura.
// #todo
// Fazer uma lista de dispositivos Intel suportados por esse driver.
// +usar if else.

    data = (uint32_t) diskReadPCIConfigAddr ( bus, dev, fun, 0 );

    Vendor = (unsigned short) (data       & 0xffff);
    Device = (unsigned short) (data >> 16 & 0xffff);

    if ( Vendor != 0x8086 || Device != 0x100E )
    {
        debug_print ("e1000_init_nic: [FAIL] Device not found\n");
        panic       ("e1000_init_nic: [FAIL] Device not found\n");
        // #bugbug: Maybe only return.
        return (int) (-1);
    }

// #debug
    printf ("Vendor=%x ",   (data       & 0xffff) );
    printf ("Device=%x \n", (data >> 16 & 0xffff) );

// pci_device structure.
// pci device struct
// passado via argumento. 

    if ( (void *) pci_device ==  NULL ){
        panic ("e1000_init_nic: pci_device\n");
    }

    // We can to this at the end of this routine.
    pci_device->used = TRUE;
    pci_device->magic = 1234;

    pci_device->bus  = (unsigned char) bus;
    pci_device->dev  = (unsigned char) dev;
    pci_device->func = (unsigned char) fun;

    pci_device->Vendor = (unsigned short) (data       & 0xffff);
    pci_device->Device = (unsigned short) (data >> 16 & 0xffff);

// #IMPORTANTE
// #bugbug:
// Esse driver é para placa Intel, vamos cancelar a inicialização 
// do driver se a placa não for Intel.
// 8086:100e | 82540EM Gigabit Ethernet Controller
// #todo
// Fazer uma lista de dispositivos Intel suportados por esse driver.
// +usar if else.
// já fizemos essa checagem antes.

    if ( pci_device->Vendor != 0x8086 || 
         pci_device->Device != 0x100E )
    {
        panic ("e1000_init_nic: 82540EM not found\n");
        // #bugbug: Maybe only return.
        return (int) (-1);
    }

// BARs

    pci_device->BAR0 = 
        (unsigned long) diskReadPCIConfigAddr( bus, dev, fun, 0x10 );
    pci_device->BAR1 = 
        (unsigned long) diskReadPCIConfigAddr( bus, dev, fun, 0x14 ); 
    pci_device->BAR2 = 
        (unsigned long) diskReadPCIConfigAddr( bus, dev, fun, 0x18 );
    pci_device->BAR3 = 
        (unsigned long) diskReadPCIConfigAddr( bus, dev, fun, 0x1C );
    pci_device->BAR4 = 
        (unsigned long) diskReadPCIConfigAddr( bus, dev, fun, 0x20 );
    pci_device->BAR5 = 
        (unsigned long) diskReadPCIConfigAddr( bus, dev, fun, 0x24 );

// IRQ

// irq
    pci_device->irq_line = 
        (uint8_t) pciConfigReadByte ( bus, dev, fun, 0x3C );
// letras
    pci_device->irq_pin = 
        (uint8_t) pciConfigReadByte ( bus, dev, fun, 0x3D ); 

// The physical address!
// #importante:
// Grab the Base I/O Address of the device
// Aqui nós pegamos o endereço dos registadores na BAR0,
// Então mapeamos esse endereço físico para termos um 
// endereço virtual para manipularmos os registradores. 
//#bugbug: size 32bit 64bit?

    phy_address = (unsigned long) ( pci_device->BAR0 & 0xFFFFFFF0 );

    if (phy_address == 0){
        panic ("e1000_init_nic: Invalid phy_address\n");
    }
  
    // ...

// Base address
// #importante:
// Mapeando para obter o endereço virtual que 
// o kernel pode manipular.
// pages.c
// #bugbug: 
// >> Isso é um improviso. Ainda falta criar rotinas melhores.

    virt_address = 
        (unsigned long) __mapping_nic1_device_address(phy_address);

    if (virt_address == 0){
        panic ("e1000_init_nic: Invalid virt_address\n");
    }

// Endereço base.
// Preparando a mesma base de duas maneiras.
    unsigned char *base_address   = (unsigned char *) virt_address;
    unsigned long *base_address32 = (unsigned long *) virt_address;

//
// == NIC =========================
//

// #todo: 
// Checar essa estrutura.
// see: nicintel.h

    currentNIC = 
        (void *) kmalloc( sizeof( struct intel_nic_info_d ) );

    if ( (void *) currentNIC ==  NULL ){
        panic ("e1000_init_nic: currentNIC struct\n");
    } 

    currentNIC->used = TRUE;
    currentNIC->magic = 1234;

    currentNIC->interrupt_count = 0;

    currentNIC->pci = (struct pci_device_d *) pci_device;

// #bugbug: Using 32bit address?
// Salvando o endereço para outras rotinas usarem.
    currentNIC->registers_base_address = (unsigned long) &base_address[0];
    currentNIC->mem_base = (uint32_t) &base_address[0];

    currentNIC->use_io = 0; //False;

//
// Get info.
//

// Device status.
    currentNIC->DeviceStatus = base_address[0x8];

//
// ## EEPROM ##
//

// False:
// Como ainda não sabemos, vamos dizer que não.
    currentNIC->eeprom = 0; 

// Let's try to discover reading the status field!

    for ( i=0; 
          i < 1000 && !currentNIC->eeprom; 
          ++i ) 
    {
        Val = __E1000ReadCommand ( currentNIC, 0x14 );

        // We have? Yes!.
        if ( (Val & 0x10) == 0x10){
            currentNIC->eeprom = 1; 
        }
    };

//
// ## MAC ##
//

// Let's read the MAC Address!

    // We can use the EEPROM!
    if (currentNIC->eeprom == 1) 
    {
        //printf("MAC from eeprom \n");  
        //refresh_screen();
        //while(1){}
 
        uint32_t tmp = __E1000ReadEEPROM ( currentNIC, 0 );
        currentNIC->mac_address[0] = (uint8_t)(tmp & 0xFF);
        currentNIC->mac_address[1] = (uint8_t)(tmp >> 8);

        tmp = __E1000ReadEEPROM ( currentNIC, 1);
        currentNIC->mac_address[2] = (uint8_t)(tmp & 0xFF);
        currentNIC->mac_address[3] = (uint8_t)(tmp >> 8);

        tmp = __E1000ReadEEPROM ( currentNIC, 2);
        currentNIC->mac_address[4] = (uint8_t)(tmp & 0xFF);
        currentNIC->mac_address[5] = (uint8_t)(tmp >> 8);

    // We can't use the EEPROM :(
    }else{

        //printf("MAC from registers \n"); 
        //refresh_screen();
        //while(1){}

        // MAC - pegando o mac nos registradores.
        currentNIC->mac_address[0] = base_address[ 0x5400 + 0 ];
        currentNIC->mac_address[1] = base_address[ 0x5400 + 1 ];
        currentNIC->mac_address[2] = base_address[ 0x5400 + 2 ];
        currentNIC->mac_address[3] = base_address[ 0x5400 + 3 ];
        currentNIC->mac_address[4] = base_address[ 0x5400 + 4 ];
        currentNIC->mac_address[5] = base_address[ 0x5400 + 5 ];
    };

// ## bus mastering ##
// Let's enable bus mastering!
// #define PCI_COMMAND 0x04
// We really need to do it?
// Yes, set the bus mastering bit
// And write back 
//( bus, slot, func, PCI_COMMAND )

    uint16_t cmd=0;
    cmd = 
        (uint16_t) pciConfigReadWord ( 
                       (unsigned char) bus, 
                       (unsigned char) dev, 
                       (unsigned char) fun, 
                       (unsigned char) 0x04 );

    // IN: (bus, slot, func, PCI_COMMAND, cmd);
    if ( (cmd & 0x04) != 0x04 )
    {
        cmd |= 0x04;
        diskWritePCIConfigAddr ( 
            (int) bus, (int) dev, (int) fun, 
            (int) 0x04, (int) cmd ); 
    }


// irq line:

    unsigned char irq_line = 
        (unsigned char) pciGetInterruptLine(bus,dev);
    printf ("Done irqline %d\n",irq_line);
    refresh_screen();

    __e1000_setup_irq(irq_line);

// Reset the controller.
    __e1000_reset_controller();
    
    e1000_interrupt_flag = TRUE;

    //printf ("e1000_init_nic: Test #breakpoint\n");
    //refresh_screen();
    //while(1){ asm("hlt"); }
    
// 0 = no errors
    return 0;
}


static void 
on_receiving ( 
    const unsigned char *buffer, 
    ssize_t size )
{
    struct e1000_ether_header_d *eth = 
        (struct e1000_ether_header_d *) buffer;
    uint16_t Type=0;


    if ( (void*) buffer == NULL )
        return;
    if(size<0)
        return;

 
    printf("\n");
    printf("Ethernet Header\n");

    if ( (void*) eth == NULL )
        return;

    // Destination MAC
    // Source MAC
    // Protocol type.

    printf ("   |-Destination Address : %x-%x-%x-%x-%x-%x \n", 
        eth->dst[0], eth->dst[1], eth->dst[2], 
        eth->dst[3], eth->dst[4], eth->dst[5] );

    printf ("   |-Source Address      : %x-%x-%x-%x-%x-%x \n", 
        eth->src[0], eth->src[1], eth->src[2], 
        eth->src[3], eth->src[4], eth->src[5] );

    printf ("   |-Ethertype           : %x \n",
        (unsigned short) eth->type);
    
    Type = (uint16_t) e1000_FromNetByteOrder16(eth->type);

    switch ( (uint16_t) Type ){
    case 0x0800:
        printf ("[0x0800]: IPV4 received\n");
        network_handle_ipv4(buffer,size);
        break;
    case 0x0806:
        printf ("[0x0806]: ARP received\n");
        network_handle_arp(buffer,size);
        break;
    case 0x814C:
        printf ("[0x814C]: SNMP received\n");
        break;
    case 0x86DD:
        printf ("[0x86DD]: IPV6 received\n");
        break;
    case 0x880B:
        printf ("[0x880B]: PPP received\n");
        break;
    };

    refresh_screen();
}


static void DeviceInterface_e1000(void)
{
    unsigned char *buffer;

    uint32_t status=0;

    uint32_t val=0;
    uint16_t old=0;
    uint32_t len=0;

// The ethernet header.
    struct e1000_ether_header_d *eh;
    uint16_t Type=0;

// Without this, the card may spam interrupts.
    __E1000WriteCommand( currentNIC, 0xD0, 1 );

// Status
    status = __E1000ReadCommand( currentNIC, 0xC0 ); 

    // 0x04 - Linkup
    // Start link.
    if (status & 0x04){
        printf ("DeviceInterface_e1000: Start link\n");
        refresh_screen();
        val = __E1000ReadCommand ( currentNIC, 0 );
        __E1000WriteCommand ( currentNIC, 0, val | 0x40 );
        return;
    // 0x80 - Reveive.
    } else if (status & 0x80){

        //printf ("DeviceInterface_e1000: Receive\n");
        //refresh_screen();

        // #lock
        // see: pciHandleDevice in pci.c

        //if (e1000_interrupt_flag == FALSE)
        //{
        //    printf("DeviceInterface_e1000: LOCKED\n");
        //    refresh_screen();
        //    return;
        //}

        // #todo
        // Esse sequência está funcionando. Não mudar.
        // Precisamos entender ela melhor.
        // Todos os buffers de recebimento.
        // Olhamos um bit do status de todos os buffers.
        // Sairemos do while quando encontrarmos um buffer com o bit desativado.
   
        while ( (currentNIC->legacy_rx_descs[currentNIC->rx_cur].status & 0x01) == 0x01 ) 
        {
             old = currentNIC->rx_cur;
             len = currentNIC->legacy_rx_descs[old].length;

             //#test: Apenas pegando o buffer para usarmos logo adinate.
             buffer = (unsigned char *) currentNIC->rx_descs_virt[old];

            //#bugbug: Não mais chamaremos a rotina de tratamento nesse momento.
            //chamaremos logo adiante, usando o buffer que pegamos acima.

            // Our Net layer should handle it
            // NetHandlePacket(dev->ndev, len, (PUInt8)dev->rx_descs_virt[old]);

            // zera.
            currentNIC->legacy_rx_descs[old].status = 0;

            // ?? Provavelmente seleciona o buffer antes de circular.
            __E1000WriteCommand ( currentNIC, 0x2818, old );
            

             // Se o bit de statos estava acionado, então copiamos esse
             // buffer para outro acessível pelos aplicativos.
             
             // Envia para o buffer do gramado.
             //if (____network_late_flag == TRUE)
             //{
                 //network_buffer_in ( (void *) buffer, (int) len );
                 //printf("DeviceInterface_e1000: [DEBUG] iret\n");
                 //refresh_screen();
             //}  
             
             //#test buffer
             if ( (void*) buffer != NULL )
             {
                 //eh = (void*) buffer;
                 on_receiving ( 
                     (const unsigned char*) buffer, 
                     1500 );
             }
             

             // circula. (32 buffers)
             // Seleciona o próximo buffer.
             currentNIC->rx_cur = (currentNIC->rx_cur + 1) % RECEIVE_BUFFER_MAX; 
            
             return;
        };
    };
}


/*
 *******************************************
 *    >>>> HANDLER <<<<
 *******************************************
 * irq_E1000:
 *     Esse é o handler da interrupção para o NIC intel 8086:100E.
 *     Esse é o driver do controlador, ele não atua sobre protocolos 
 * de rede, então deve-se enviar uma mensagem para o servidor de rede 
 * para ele analizar o conteúdo do buffer, para assim decidir qual 
 * é o protocolo e redirecionar para a rotina de tratamento do 
 * protocolo específico.
 *     Esse é o driver do controlador, ele deve solicitar ao kernel
 * qual é o PID do processo que é o servidor de rede, e enviar
 * a mensagem para ele, contendo o endereço do buffer.
 */
// Isso é chamado pelo assembly.

__VOID_IRQ 
irq_E1000 (void)
{
    gE1000InputTime = (unsigned long) jiffies;  
    DeviceInterface_e1000();
}










