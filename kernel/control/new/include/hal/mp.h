/*
 * File: mp.h 
 *     Multi-processor (MP) support.
 *     Symmetric multiprocessing (SMP) 
 */

// See:
// https://wiki.osdev.org/Symmetric_Multiprocessing
// https://wiki.osdev.org/User:Shikhin/Tutorial_SMP
// apic.h
// https://www.cheesecake.org/sac/smp.html
// https://en.wikipedia.org/wiki/Symmetric_multiprocessing


#ifndef ____MP_H
#define ____MP_H    1


/*
First, you need to find the floating pointer structure. 
According to the spec, it can be in one of four places: 
(1) in the first kilobyte of the extended BIOS data area, 
(2) the last kilobyte of base memory, 
(3) the top of physical memory, or 
(4) the BIOS read-only memory space between 0xe0000 and 0xfffff. 
You need to search these areas for the four-byte 
signature "_MP_" which denotes the start of the 
floating pointer structure. 
*/

// See
// https://pdos.csail.mit.edu/6.828/2008/readings/ia32/MPspec.pdf

struct mp_floating_pointer_structure 
{

// The Signature, must contain _MP_, and 
// is present on a 16 byte boundary.
    char signature[4];

// The address of the MP Configuration Table.
    uint32_t configuration_table;
    
// The length of the floating point structure table, in 16 byte units. 
// This field *should* contain 0x01, meaning 16-bytes.
// In 16 bytes (e.g. 1 = 16 bytes, 2 = 32 bytes)
    uint8_t length; 
    
// The version number of the MP Specification. 
// A value of 1 indicates 1.1, 4 indicates 1.4, and so on.
    uint8_t mp_specification_revision;
    
// The checksum of the Floating Point Structure.
// This value should make all bytes in the table equal 0 when added together
    uint8_t checksum; 

// If this is not zero then configuration_table should be 
// ignored and a default configuration should be loaded instead    
    uint8_t default_configuration; 

// Few feature bytes.
// If bit 7 is set then 
// the IMCR is present and PIC mode is being used, otherwise 
// virtual wire mode is; 
// all other bits are reserved
    uint32_t features; 
};
struct mp_floating_pointer_structure *MPTable;


struct mp_configuration_table 
{
    char signature[4]; // "PCMP"
    uint16_t length;
    uint8_t mp_specification_revision;
    uint8_t checksum; // Again, the byte should be all bytes in the table add up to 0
    char oem_id[8];
    char product_id[12];
    uint32_t oem_table;
    uint16_t oem_table_size;
    uint16_t entry_count; // This value represents how many entries are following this table

// This is the memory mapped address of the local APICs
// The base address by which each processor accesses
// its local APIC.

    // MEMORY-MAPPED ADDRESS OF LOCAL APIC
    uint32_t lapic_address;

    uint16_t extended_table_length;
    uint8_t extended_table_checksum;
    uint8_t reserved;
};
struct mp_configuration_table *MPConfigurationTable;


// #
// Logo abaixo da estrutura acima segue uma
// sequencia de entadas de tipo e tamanhos diferentes.
// As que descrevem processadores possuem 20bytes
// as outras possuem 8 bytes.

/*
entry info:
Description | Type | Length | Comments
Processor  |     0 |     20 | One entry per processor.
Bus        |     1 |      8 | One entry per bus.
I/O APIC   |     2 |      8 | One entry per I/O APIC.
I/O 
Interrupt 
Assignment |     3 |      8 | One entry per bus interrupt source.
Local 
Interrupt 
Assignment |     4 |      8 | One entry per system interrupt source.
*/


//# size: 20 bytes
struct entry_processor 
{
    uint8_t type; // Always 0
    uint8_t local_apic_id;
    uint8_t local_apic_version;

// flags:
// If bit 0 is clear then the processor must be ignored.
// If bit 1 is set then the processor is the bootstrap processor.
    uint8_t flags;

// Stepping, model, family
    uint32_t signature;
    
    uint32_t feature_flags;
    
    unsigned long reserved;
    //uint64_t reserved;
};



/*
// # size: 8 bytes
struct entry_io_apic 
{
    uint8_t type; // Always 2
    uint8_t id;
    uint8_t version;
    uint8_t flags; // If bit 0 is set then the entry should be ignored
    
    uint32_t address; // The memory mapped address of the IO APIC is memory
};
*/

//
// prototypes ==========
//

// See: x64.c
void smp_probe(void);

#endif   



//
// End.
//


