
// atapci.c


#include <kernel.h>


// #todo:
// Checar se temos uma lista dessa no suporte a PCI.

const char *pci_classes[] = {
    "Unknown [old]",
    "Mass storage",
    "Network",
    "Display",
    "Multimedia device",
    "Memory",
    "Bridge device",
    "Simple Communication",
    "Base System Peripheral",
    "Input Device",
    "Docking Station",
    "Processor",
    "Serial Bus",
    "Wireless",
    "Inteligent I/O",
    "Satellite Communications",
    "Encrypt/Decrypt",
    "Data acquisition and signal processing",
    [255]="Unknown"
};




/* 
 * diskReadPCIConfigAddr:
 *     READ 
 */

uint32_t 
diskReadPCIConfigAddr ( 
    int bus, 
    int dev,
    int fun, 
    int offset )
{
    out32 ( PCI_PORT_ADDR, CONFIG_ADDR( bus, dev, fun, offset ) );

    return (uint32_t) in32 (PCI_PORT_DATA);
}


/* 
 * diskWritePCIConfigAddr:
 *     WRITE 
 */

void 
diskWritePCIConfigAddr ( 
    int bus, 
    int dev,
    int fun, 
    int offset, 
    int data )
{
    out32 ( PCI_PORT_ADDR, CONFIG_ADDR( bus, dev, fun, offset ) );
    out32 ( PCI_PORT_DATA, data );
}


/*
 * diskPCIScanDevice:
 *     Busca um dispositivo de acordo com a classe.  
 *     Esta função deve retornar uma varia'vel contendo: 
 *     + O número de barramento, 
 *     + o dispositivo e  
 *     + a função.
 * IN: Class.
 * OUT: data.
 *      -1 = error (#bugbug, pois o tipo de retorno eh unsigned int)
 */

uint32_t diskPCIScanDevice ( int class )
{
    int bus=0;
    int dev=0;
    int fun=0;

// #bugbug 
// Usando -1 para unsigned int. 

    uint32_t data = -1;

//#ifdef KERNEL_VERBOSE
    //printf ("diskPCIScanDevice:\n");
    //refresh_screen ();
//#endif

// Probe.

    for ( bus=0; bus < 256; bus++ )
    {
        for ( dev=0; dev < 32; dev++ )
        {
            for ( fun=0; fun < 8; fun++ )
            {
                out32 ( PCI_PORT_ADDR, CONFIG_ADDR( bus, dev, fun, 0x8) );

                data = in32 (PCI_PORT_DATA);
                
                if ( ( data >> 24 & 0xff ) == class )
                {

//#ifdef KERNEL_VERBOSE
//                    printf ("Detected PCI device: %s \n", 
//                        pci_classes[class] );
//#endif 
                    // Done !

                    return (uint32_t) ( fun + (dev*8) + (bus*32) );
                }
            };
        };
    };


// Fail
    printf ("diskPCIScanDevice: PCI device NOT detected\n");

//#bugbug
//isso e' lento
    //refresh_screen();

    return (uint32_t) (-1);
}


/*
 * atapciConfigurationSpace:
 *     Espaço de configuraçao PCI Mass Storage.
 */

// Nessa rotina:
// + Encontra o tipo de driver, ser é IDE, RAID, AHCI ou Desconhecido.

int atapciConfigurationSpace ( struct pci_device_d *D )
{
    uint32_t data=0;

// A estrutura ainda nao foi configurada.
    ata.used = FALSE;
    ata.magic = 0;

// #type: FAIL
    ata.chip_control_type = ATA_UNKNOWN_CONTROLLER;


// Check parameters.

    if ( (void *) D == NULL ){
        printf ("atapciConfigurationSpace: D struct\n");
        goto fail;
    }

    if ( D->used != TRUE || 
         D->magic != 1234 )
    {
        printf ("atapciConfigurationSpace: D validation\n");
        goto fail;
    }

// Indentification Device
    //data = (uint32_t) diskReadPCIConfigAddr ( D->bus, D->dev, D->func, 0 );

// Salvando configurações.
    //D->Vendor = data &0xffff;
    //D->Device = data >> 16 &0xffff;

//#ifdef KERNEL_VERBOSE
//    printf ("Disk info: [ Vendor ID: %x,Device ID: %x ]\n", 
//        D->Vendor, D->Device );
//#endif


    /*
    if ( D->Vendor == 0x1106 && D->Device == 0x0591 )
    {
        kprintf ("VIA disk found\n");
    } else if (D->Vendor == 0x1106 && D->Device == 0x0591){
        // ...
    };
    */

// Obtendo informações.
// Classe code, programming interface, revision id.

    //data  = (uint32_t) diskReadPCIConfigAddr ( D->bus, D->dev, D->func, 8 );

// Saving info.
// Classe e sub-classe.
// prog if.
// Revision.

    //ata_pci->classCode  = data >> 24 & 0xff;
    //ata_pci->subclass   = data >> 16 & 0xff;
    //ata_pci->progif     = data >> 8  & 0xff;
    //ata_pci->revisionId = data       & 0xff;

// #importante:
// Aqui detectamos o tipo de dispositivo com base 
// nas informações de classe e subclasse.

//
// == #IDE ========
//


// ====
    if ( D->classCode == PCI_CLASSCODE_MASS && D->subclass == PCI_SUBCLASS_IDE ){

        // #type: (IDE).
        ata.chip_control_type = ATA_IDE_CONTROLLER; 

        // Compatibilidade e nativo, primary.
        data  = diskReadPCIConfigAddr ( D->bus, D->dev, D->func, 8 );
        if ( data & 0x200 ){ 
            diskWritePCIConfigAddr ( D->bus, D->dev, D->func, 8, data | 0x100 ); 
        }

        // Compatibilidade e nativo, secundary.
        data = diskReadPCIConfigAddr ( D->bus, D->dev, D->func, 8 );
        if ( data & 0x800 ){ 
            diskWritePCIConfigAddr ( D->bus, D->dev, D->func, 8, data | 0x400 ); 
        }

        //#todo: Comment.
        data = diskReadPCIConfigAddr ( D->bus, D->dev, D->func, 8 );
        if ( data & 0x8000 ){
            // Bus Master Enable
            data = diskReadPCIConfigAddr (D->bus, D->dev, D->func, 4);
            diskWritePCIConfigAddr ( D->bus, D->dev, D->func, 4, data | 0x4);
        }

        // Habilitar interrupcao (INTx#)
        data = diskReadPCIConfigAddr ( D->bus, D->dev, D->func, 4 );
        diskWritePCIConfigAddr ( D->bus, D->dev, D->func, 4, data & ~0x400);

        // IDE Decode Enable
        data = diskReadPCIConfigAddr ( D->bus, D->dev, D->func, 0x40 );
        diskWritePCIConfigAddr ( D->bus, D->dev, D->func, 0x40, data | 0x80008000 );

        // Synchronous DMA Control Register
        // Enable UDMA
        data = diskReadPCIConfigAddr ( D->bus, D->dev, D->func, 0x48 );
        diskWritePCIConfigAddr ( D->bus, D->dev, D->func, 0x48, data | 0xf);


//#ifdef KERNEL_VERBOSE 
        //kprintf ("[ Sub Class Code %s Programming Interface %d Revision ID %d ]\n",\
        //    ata_sub_class_code_register_strings[ata.chip_control_type],
        //    ata_pci.progif,
        //    ata_pci.revisionId );
//#endif


//
// == #RAID ========
//

// #todo
// Devemos falhar, 
// pois não daremos suporte à IDE RAID.

// ====
    }else if ( D->classCode == PCI_CLASSCODE_MASS && D->subclass == PCI_SUBCLASS_RAID ){

        // #type: (ATA RAID).
        ata.chip_control_type = ATA_RAID_CONTROLLER;
              
        printf ("atapciConfigurationSpace: ATA RAID not supported\n");
        goto fail;
  
//
//  # ACHI
//

// #todo
// Devemos falhar, pois temos outro driver 
// para esse tipo de controlador.

// ====
    }else if ( D->classCode == PCI_CLASSCODE_MASS && D->subclass == PCI_SUBCLASS_SATA ){

        // #type (ACHI)
        ata.chip_control_type = ATA_AHCI_CONTROLLER;

        // Compatibilidade e nativo, primary.
        data = diskReadPCIConfigAddr ( D->bus, D->dev, D->func, 8 );
        if ( data & 0x200 ){ 
            diskWritePCIConfigAddr ( D->bus, D->dev, D->func, 8, data | 0x100 ); 
        }

        // Compatibilidade e nativo, secundary.
        data = diskReadPCIConfigAddr ( D->bus, D->dev, D->func, 8 );
        if ( data & 0x800 ){ 
            diskWritePCIConfigAddr ( D->bus, D->dev, D->func, 8, data | 0x400 ); 
        }

        // ??#todo: Comment
        data = diskReadPCIConfigAddr ( D->bus, D->dev, D->func, 8 );
        if ( data & 0x8000 ) 
        {    
            // Bus Master Enable.
            data = diskReadPCIConfigAddr ( D->bus, D->dev, D->func, 4 );
            diskWritePCIConfigAddr ( D->bus, D->dev, D->func, 4, data | 0x4 );
        } 

        // IDE Decode Enable.
        data = diskReadPCIConfigAddr ( D->bus, D->dev, D->func, 0x40 );
        diskWritePCIConfigAddr ( D->bus, D->dev, D->func, 0x40, data | 0x80008000 );

        // Habilitar interrupcao (INTx#)
        data = diskReadPCIConfigAddr ( D->bus, D->dev, D->func, 4 );
        diskWritePCIConfigAddr ( D->bus, D->dev, D->func, 4, data & ~0x400);

//#ifdef KERNEL_VERBOSE
        // kprintf ("[ Sub Class Code %s Programming Interface %d Revision ID %d ]\n",\
        //     ata_sub_class_code_register_strings[ata.chip_control_type], 
        //     ata_pci.progif,
        //     ata_pci.revisionId );
//#endif


//
// No type
//

// Fail!
// O tipo de dispositivo de armazenaento de massa 
// não é suportado.

// ====
    }else{
        
        // #type: FAIL
        ata.chip_control_type = ATA_UNKNOWN_CONTROLLER;
        
        printf("atapciConfigurationSpace: Mass Storage Device NOT supported\n");
        goto fail;
    };

// #obs:
// Nesse momento já sabemos se é IDE, RAID, AHCI.
// Vamos pegar mais informações,
// Salvaremos as informações na estrutura.

// PCI cacheline, Latancy, Headr type, end BIST
    data = diskReadPCIConfigAddr ( D->bus, D->dev, D->func, 0xC );
    D->latency_timer = (data >> 8)  & 0xff;
    D->header_type   = (data >> 16) & 0xff;
    D->bist          = (data >> 24) & 0xff;

// BARs
    D->BAR0 = diskReadPCIConfigAddr ( D->bus, D->dev, D->func, 0x10 );
    D->BAR1 = diskReadPCIConfigAddr ( D->bus, D->dev, D->func, 0x14 );
    D->BAR2 = diskReadPCIConfigAddr ( D->bus, D->dev, D->func, 0x18 );
    D->BAR3 = diskReadPCIConfigAddr ( D->bus, D->dev, D->func, 0x1C );
    D->BAR4 = diskReadPCIConfigAddr ( D->bus, D->dev, D->func, 0x20 );
    D->BAR5 = diskReadPCIConfigAddr ( D->bus, D->dev, D->func, 0x24 );

// irqline and irq pin.
    data = diskReadPCIConfigAddr ( D->bus, D->dev, D->func, 0x3C );
    D->irq_line = (data & 0xff);
    D->irq_pin  = (data >> 8) & 0xff;

// PCI command and status.
    data = diskReadPCIConfigAddr ( D->bus, D->dev, D->func, 4 );
    D->Command = (data & 0xffff); 
    D->Status  = (data >> 16) & 0xffff;

// #debug
//#ifdef KERNEL_VERBOSE
//    printf ("[ Command %x Status %x ]\n", D->Command, D->Status );
//    printf ("[ Interrupt Line %x Interrupt Pin %x ]\n", 
//        D->irq_line, D->irq_pin );
//#endif


//
// == DMA ====================
//
    data = diskReadPCIConfigAddr ( D->bus, D->dev, D->func, 0x48);
//#debug
    //printf ("[ Synchronous DMA Control Register %x ]\n", data );

//done:
    ata.used = TRUE;
    ata.magic = 1234;
    return (int) PCI_MSG_SUCCESSFUL;

fail:
    refresh_screen();

    ata.used = FALSE;
    ata.magic = 0;
    return (int) PCI_MSG_ERROR;
}

