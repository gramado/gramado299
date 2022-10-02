
// x64.c

#include <kernel.h>    


// FPU
// To initialize the x87 FPU, 
// set CR0.MP and CR0.NE, 
// clear CR0.EM and CR0.TS, and 
// execute FNINIT. 


/*
 // 8259 isa irqs
unsigned char isa_irqs[16] = {
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
    0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F  
};
*/


/*
 * x64_init_gdt:
 *     It creates a TSS and sets up some entries in the GDT.
 *     See: x86gdt.h
 */

// Called by I_x64main in init.c.
// See: head_64.asm

extern void rsp0Stack(void);
extern void gdt_flush(unsigned long gdtr_address);

int x64_init_gdt(void)
{
    struct tss_d  *tss;

    // debug_print ("[x64] x64_init_gdt: [DANGER] \n");

// #danger
// limpando gdt.

    memset(
        &xxx_gdt[GNULL_SEL],
        0,
        sizeof(struct segment_descriptor_d)*32 );

// IN: 
// (entry address, limit, base, type, s, dpl, p, avl, l, db, g)

// null
    set_gdt_entry ( &xxx_gdt[GNULL_SEL], 
        0,0,0,0,0,0,0,0,0,0);

// ring 0
// dpl 0
// (n, limit, base, type, s, dpl, p, avl, l, db, g)
    set_gdt_entry ( 
        &xxx_gdt[GCODE_SEL], 
        0,    // limit
        0x0,  // base
        SEG_CODE_EXRD, //0xA,  // type
        1,    // s
        RING0, //0,    // dpl
        1,    // p
        0,    // avl
        1,    // l
        0,    // db
        0);   // g
    set_gdt_entry ( 
        &xxx_gdt[GDATA_SEL], 
        0,    // limit
        0x0,  // base
        SEG_DATA_RDWR, //0x2,  // type
        1,    // s
        RING0, //0,    // dpl
        1,    // p
        0,    // avl
        1,    // l
        0,    // db
        0);   // g

// ring 3
// dpl 3
// (n, limit, base, type, s, dpl, p, avl, l, db, g)
    set_gdt_entry ( 
        &xxx_gdt[GUCODE_SEL], 
        0,    // limit
        0x0,  // base
        SEG_CODE_EXRD, //0xA,  // type
        1,    // s
        RING3, //3,    // dpl ??
        1,    // p
        0,    // avl
        1,    // l
        0,    // db
        0);   // g
    set_gdt_entry ( 
        &xxx_gdt[GUDATA_SEL], 
        0,      // limit   
        0x0,    // base
        SEG_DATA_RDWR, //0x2,    // type
        1,      // s
        RING3, //3,      // dpl ??
        1,      // p
        0,      // avl
        1,      // l
        0,      // db
        0);     // g

//
// tss
//

// Creating a TSS and initializing it.
// Save current tss.
// Create gdt entry for the tss. (two entries)

    tss = (void *) kmalloc ( sizeof(struct tss_d) );

    if ( (void *) tss == NULL ){
        debug_print ("[x64] x64_init_gdt: \n");
              panic ("[x64] x64_init_gdt: \n");
    }

    tss_init ( 
        (struct tss_d *) tss,  // tss 
        (void *) &rsp0Stack    // ring 0 stack address
        );

    CurrentTSS = tss;

    set_gdt_entry ( &xxx_gdt[GTSS_SEL], 
        sizeof( struct tss_d ) - 1, (unsigned long) tss,0x9,0,3,1,0,0,0,1); //tss dpl 3
    set_gdt_entry ( &xxx_gdt[GTSS_CONT_SEL], 
        (unsigned long) tss >> 32, (unsigned long) tss >> 48,0,0,0,0,0,0,0,0);

//
// Load GDT.
//

// Limit and base.

    xxx_gdt_ptr.limit = 
        (unsigned short) ((DESCRIPTOR_COUNT_MAX * sizeof(struct segment_descriptor_d) ) -1);
    xxx_gdt_ptr.base = 
        (unsigned long) &xxx_gdt[GNULL_SEL];


// register.
    gdt_flush( (unsigned long) &xxx_gdt_ptr );

    // See: x64gdt.h
    //load_gdt (&xxx_gdt_ptr);

// #todo
// print gdt entries.

//
// Load tr   [DANGER]
//

// Load TR.
// 0x2B = (0x28+3).
// #bugbug
// Falha quando carregamos isso.

    x64_load_ltr(0x2B);

    return 0;
}


// Set segment.
// Probably stolen from minix or netbsd.
// See: x64gdt.h

void
set_gdt_entry ( 
    struct segment_descriptor_d *sd, 
    unsigned int limit,
    unsigned int base,
    unsigned char type,
    unsigned char s, 
    unsigned char dpl, 
    unsigned char p, 
    unsigned char avl,
    unsigned char l,
    unsigned char db,   //Sz 
    unsigned char g )
{


    if ( (void*) sd == NULL )
    {
        debug_print ("[x64] set_gdt_entry: sd\n");
        panic       ("[x64] set_gdt_entry: sd\n");
    }

// limit
    sd->limit_15_0  = (limit & 0xFFFF);       // (16) segment extent (lsw) 
    sd->limit_19_16 = ((limit >> 16) & 0xF);  // (4)

// base
    sd->base_15_0   = (base & 0xFFFF);         // (16) lsw
    sd->base_23_16  = ((base >> 16) & 0xFF);   // (8)
    sd->base_31_24  = ((base  >> 24) & 0xFF);  // (8)

// access byte
    sd->type  = ( type & 0xF );  // (4)
    sd->s     = ( s    & 0x1 );  // (1)
    sd->dpl   = ( dpl  & 0x3 );  // (2)
    sd->p     = ( p    & 0x1 );  // (1)

//
    sd->avl  = (avl & 1);  // (1)  
    sd->l    = (l   & 1);  // (1)
    sd->db   = (db  & 1);  // (1)
    sd->g    = (g   & 1);  // (1)
}



// ======================


void
tss_init ( 
    struct tss_d *tss, 
    void *stack_address )
{

    if ( (void *) tss == NULL )
    {
        debug_print ("[x64] tss_init:\n");
        panic       ("[x64] tss_init:\n");
    }

    // Clean.
    memset ( tss, 0, sizeof *tss );

    //ring 0 stack
    tss->rsp0 = (unsigned long) stack_address;  // va ?? 

    //#debug
    //printf ("Stack %x\n", stack_address);
    //refresh_screen();
    //while(1){}
}


// # not tested yet
void x64_load_ltr (int tr)
{
    asm volatile ( \
        " movw %w0, %%ax; \
          ltrw %%ax;     "\
        :: "a"(tr) );
}

// ==========================



/*
 * get_cpu_intel_parameters:
 *     Pega os parâmetros da cpu x64 através da instrução cpuid.
 *
 * Vendor               = 0.
 * Apic ??              = 1.
 * Number of cores      = 4.
 * Name                 = 0x80000002,0x80000003,0x80000004. (Processor Brand String).
 * Max feature id       = 0x80000000.
 * L2 cache information = 0x80000006.(Extended L2 Cache Features)
 * core per die         = 0x80000008.
 */

// #todo:
// apic
// The CPUID.01h:EDX[bit 9] flag 
// specifies whether a CPU has a built-in local APIC. 

// According to the Intel CPUID application note, 
// we should first check the Vendor ID String for "GenuineIntel" 
// before taking out information, such as 
// the Processor Signature, Processor Feature Flags, etc. 

void get_cpu_intel_parameters (void)
{
    unsigned long eax=0; 
    unsigned long ebx=0; 
    unsigned long ecx=0;
    unsigned long edx=0;
    unsigned long name[32];
    int MASK_LSB_8 = 0xFF;

    //debug_print ("get_cpu_intel_parameters: [FIXME]\n");

//========================================
// EAX=0: Highest Function Parameter and Manufacturer ID
// #todo: eax
// eax = Maximum meaningful value for the InfoType parameter. 
// Vendor.
// ebx = Identification String (part 1)
// ecx = Identification String (part 3)
// edx = Identification String (part 2)

    cpuid ( 0, eax, ebx, ecx, edx ); 
    name[0] = ebx;
    name[1] = edx;
    name[2] = ecx;
    name[3] = 0;

    processor->Vendor[0] = ebx;
    processor->Vendor[1] = edx;
    processor->Vendor[2] = ecx;
    processor->Vendor[3] = 0;

    hal_set_machine_type(Processor_NULL);

// Intel?
// #todo: definir Intel como 1.
    if ( ebx == CPUID_VENDOR_INTEL_1 && 
         edx == CPUID_VENDOR_INTEL_2 && 
         ecx == CPUID_VENDOR_INTEL_3 )
    {
        hal_set_machine_type(Processor_INTEL);
    }


//========================================
// EAX=1: Processor Info and Feature Bits

//
// Output para argumento 1. 
//

// eax:
//  0- 3 Stepping ID
//  4- 7 Model
//  8-11 Family
// 12-13 Processor Type
// 14-15 Reserved
// 16-19 Extended model
// 20-27 Extended family
// 28-31 Reserved

// ebx:
//  0- 7 Brand Index
//  8-15 CLFLUSH cache line size / 8.
// 16-23 Reserved
// 24-31 APIC Physical ID.

// ecx:
//     0 SSE3 New Instructions
// 1-  2 Reserved
//     3 MONITOR/MWAIT
//     4 CPL Qualified Debug Store
// 5-  7 Reserved
//     8 Thermal Monitor 2
//     9 Reserved
//    10 L1 Context ID
// 11-31 Reserved

// edx: 
//  0 FPU x87 FPU on Chip.
//  1 VME Virtual-8086 Mode Enhancement
//  2 DE Debugging Extensions
//  3 PSE Page Size Extensions
//  4 TSC Time Stamp Counter
//  5 MSR RDMSR and WRMSR Support.
//  6 PAE Physical Address Extensions
//  7 MCE Machine Check Exception
//  8 CX8 CMPXCHG8B Inst.
//  9 APIC APIC on Chip.
// 10 n/a Reserved
// 11 SEP SYSENTER and SYSEXIT
// 12 MTRR Memory Type Range Registers
// 13 PGE PTE Global Bit
// 14 MCA Machine Check Architecture
// 15 CMOV Conditional Move/Compare Instruction
// 16 PAT Page Attribute Table
// 17 PSE Page Size Extension (PSE36)
// 18 PSN Processor Serial Number
// 19 CLFSH CFLUSH Instruction
// 20 n/a Reserved
// 21 DS Debug Store
// 22 ACPI Thermal Monitor and Clock Ctrl
// 23 MMX MMX Technology
// 24 FXSR FXSAVE/FXRSTOR
// 25 SSE SSE Extensions
// 26 SSE2 SSE2 Extensions
// 27 SS Self Snoop
// 28 HTT Hyper-threading technology.
// 29 TM Thermal Monitor
// 30 n/a Reserved
// 31 PBE Pend. Brk. En.

    cpuid ( 1, eax, ebx, ecx, edx );

// eax:
// Processor Version Information 
    processor->Stepping_ID = 
        (unsigned long)( eax        & 0xF);  //stepping
    processor->Model = 
        (unsigned long)((eax >> 4)  & 0xf);  //model
    processor->Family_ID = 
        (unsigned long)((eax >> 8)  & 0xf);  //family
    processor->Processor_Type = 
        (unsigned long)((eax >> 12) & 0x3);  //processor type
    processor->Extended_Model_ID = 
        (unsigned long)((eax >> 16) & 0xf);  //extended model
    processor->Extended_Family_ID = 
        (unsigned long)((eax >> 20) & 0xff); //extended family
    // 31 ~ 28 Reserved??

// ebx:
// Additional Information 

    //processor->xx = (unsigned long)((ebx >> 9) & 0x0001);
    //... 


// ecx e edx:
// Feature Information 


// ecx

// SSE3 - bit 0.
    processor->hasSSE3 = 
        (unsigned long)( ecx & 0x0001);


// edx:

// The CPUID.01h:EDX[bit 0] flag.
// Specifies whether a CPU has a built-in local x87 FPU. 
    processor->hasX87FPU = 
        (unsigned long)( edx & 0x0001);

// PSE - bit 3.
    processor->hasPSE = 
        (unsigned long)((edx >> 3) & 0x0001);

// TSC - bit 4.
    processor->hasTSC = 
        (unsigned long)((edx >> 4) & 0x0001);

//The presence of MSRs on your processor is 
//indicated by CPUID.01h:EDX[bit 5].
    processor->hasMSR = 
        (unsigned long)((edx >> 5) & 0x0001);

// PAE - bit 6.
    processor->hasPAE = 
        (unsigned long)((edx >> 6) & 0x0001);

// The CPUID.01h:EDX[bit 9] flag 
// Specifies whether a CPU has a built-in local APIC. 
    processor->hasAPIC = 
        (unsigned long)((edx >> 9) & 0x0001);


// MTRR Memory Type Range Registers
    processor->hasMTRR = 
        (unsigned long)((edx >> 12) & 0x0001);

// PAT - Page Attribute Table
    processor->hasPAT = 
        (unsigned long)((edx >> 16) & 0x0001);

// PSE - Page Size Extension (PSE36)
    processor->hasPSE36 = 
        (unsigned long)((edx >> 17) & 0x0001);

// PSN - Processor Serial Number
    processor->hasPSN = 
        (unsigned long)((edx >> 18) & 0x0001);

//---

// MMX
    processor->hasMMX = 
        (unsigned long)((edx >> 23) & 0x0001);

// SSE
    processor->hasSSE = 
        (unsigned long)((edx >> 25) & 0x0001);

// SSE2
    processor->hasSSE2 = 
        (unsigned long)((edx >> 26) & 0x0001);


// HTT - hyper-threading technology
    processor->hasHTT = 
        (unsigned long)((edx >> 28) & 0x0001);

    // ...



//========================================
// EAX=2: Cache and TLB Descriptor information
    
    // ...
    
    //========================================
    // EAX=3: Processor Serial Number
    
	//Serial number ???
	//cpuid( 3, eax, ebx, ecx, edx );


    //========================================
    // EAX=4 and EAX=Bh: Intel thread/core and cache topology
    
	//Check number of cores.
	//cpuid( 4, eax, ebx, ecx, edx );



    //========================================
    // EAX=6: Thermal and power management
    

    //========================================
    // EAX=7, ECX=0: Extended Features
    // EAX=7, ECX=1: Extended Features
    

    // ==========================================================
    // hypervisor level 4000_0000h
    cpuid ( 0x40000000, eax, ebx, ecx, edx );
    processor->hvName[0] = ebx;
    processor->hvName[1] = ecx;
    processor->hvName[2] = edx;
    processor->hvName[3] = 0;



    //========================================
    // EAX=80000000h: Get Highest Extended Function Implemented

    // Max feature id.
    cpuid ( 0x80000000, eax, ebx, ecx, edx);
    name[0] = eax;  //Maximum meaningful value of InfoType for extended function CPUID information.
    name[1] = ebx;  //reserved
    name[2] = ecx;  //reserved
    name[3] = edx;  //reserved
    name[4] = 0; 
    processor->MaxFeatureId = (unsigned long)(eax & MASK_LSB_8);
    //printf("Max feature id ={%d}\n", (unsigned long) processor->MaxFeatureId);


    //========================================
    // EAX=80000001h: Extended Processor Info and Feature Bits
    
    //========================================
    // EAX=80000002h,80000003h,80000004h: Processor Brand String
    
    /*name part 1*/
    cpuid ( 0x80000002, eax, ebx, ecx, edx);
    name[0] = eax;  //Processor Brand String
    name[1] = ebx;  //Processor Brand String
    name[2] = ecx;  //Processor Brand String
    name[3] = edx;  //Processor Brand String
    name[4] = 0;
    //salva na estrutura.
    processor->BrandName[0] = eax;
    processor->BrandName[1] = ebx;
    processor->BrandName[2] = ecx;
    processor->BrandName[3] = edx;
	//printf("CPU={%s", &name[0]);
	//printf("%s",&name[0]);		

   /*name part 2*/
    cpuid ( 0x80000003, eax, ebx, ecx, edx);
    name[0] = eax;  //Processor Brand String, continued
    name[1] = ebx;  //Processor Brand String, continued
    name[2] = ecx;  //Processor Brand String, continued
    name[3] = edx;  //Processor Brand String, continued
    name[4] = 0;
	//salva na estrutura.	
    processor->BrandName[4] = eax;
    processor->BrandName[5] = ebx;
    processor->BrandName[6] = ecx;
    processor->BrandName[7] = edx;
	//printf("%s",&name[0]);

   /*name part 3*/
    cpuid ( 0x80000004, eax, ebx, ecx, edx);
    name[0] = eax; //Processor Brand String, continued
    name[1] = ebx; //Processor Brand String, continued
    name[2] = ecx; //Processor Brand String, continued
    name[3] = edx; //Processor Brand String, continued
    name[4] = 0;
	//salva na estrutura.	
    processor->BrandName[8]  = eax;
    processor->BrandName[9]  = ebx;
    processor->BrandName[10] = ecx;
    processor->BrandName[11] = edx;
	//printf("%s}\n",&name[0]);	


    //========================================
    // EAX=80000005h: L1 Cache and TLB Identifiers




    //========================================
    // EAX=80000006h: Extended L2 Cache Features
 
    // pegamos logo acima.
    if ( processor->MaxFeatureId < 6)
    {
        debug_print ("get_cpu_intel_parameters: Cache Extended Feature not supported\n");
        //printf("Cache Extended Feature not supported\n");
        //goto done;
        return;
    }

    /*
     * L2 cache information (Intel)
     *
     * EAX Reserved
     * EBX Reserved
     * ECX Bits:  
     *     Bits 0-7: Cache Line Size.
     *     Bits 12-15: L2 Associativity.
     *     Bits 16-31: Cache size in 1K units.   
     * EDX Reserved
     */

    cpuid ( 0x80000006, eax, ebx, ecx, edx );
    name[0] = eax;
    name[1] = ebx;
    name[2] = ecx;
    name[3] = edx;
    name[4] = 0;
    processor->L2LineSize      = (unsigned long)( ecx        & 0x00FF); //Bits 0-7: Cache Line Size.
    processor->L2Associativity = (unsigned long)((ecx >> 12) & 0x000F);	//Bits 12-15: L2 Associativity.
    processor->L2Cachesize     = (unsigned long)((ecx >> 16) & 0xFFFF); //Bits 16-31: Cache size in 1K units.
	//printf("L2LineSize={%d Byte}\n",(unsigned long) processor->L2LineSize);	
	//printf("L2Cachesize={%d KB}\n",(unsigned long) processor->L2Cachesize);


    //========================================
    // EAX=80000007h: Advanced Power Management Information



    //========================================
    //EAX=80000008h: 
    //Virtual and Physical address Sizes	
    //Returns largest virtual and physical address sizes.

    /*
     * Virtual and physical memory sizes.
     */

    cpuid ( 0x80000008, eax, ebx, ecx, edx );
    name[0] = eax;    //Virtual and physical memory sizes.
    name[1] = ebx;    //reserved
    name[2] = ecx;    //reserved
    name[3] = edx;    //reserved
    name[4] = 0;
    processor->Physical_Address_Size = (unsigned long) ( eax       & 0x00FF); //7-0
    processor->Virtual_Address_Size  = (unsigned long) ((eax >> 8) & 0x00FF); //15-8	
    //printf("Physical_Address_Size={%d}\n",(unsigned long) processor->Physical_Address_Size);
    //printf("Virtual_Address_Size={%d}\n", (unsigned long) processor->Virtual_Address_Size);

    //...
}


void x64_init_fpu (void) 
{
    asm volatile ("fninit");
}


// See: unit1hw.asm
extern void DisableSSE(void);
extern void EnableSSE(void);


int x64_init_fpu_support(void)
{
    if( (void*) processor == NULL )
    {
        printf("fail: processor\n");
        return -1;
    }

    if( processor->hasX87FPU != TRUE )
    {
        printf("fail: processor->hasX87FPU\n");
        return -1;
    }

    if( processor->hasSSE != TRUE )
    {
        printf("fail: processor->hasSSE\n");
        return -1;
    }

    if( processor->hasSSE2 != TRUE )
    {
        printf("fail: processor->hasSSE2\n");
        return -1;
    }

    if( processor->hasSSE3 != TRUE )
    {
        printf("fail: processor->hasSSE3\n");
        return -1;
    }

// Enable

    asm volatile (
        " movq %%cr4, %%rax;  "
        " orl $0x600, %%eax;  "    /*Set OSFXSR and OSXMMEXCPT*/
        " movq %%rax, %%cr4;  "
        
        " movq %%cr0, %%rax;  "
        " andw $0xFFFB, %%ax; "    /*Clear EM*/
        " orw $0x2, %%ax;     "    /*Set MP*/
        " movq %%rax, %%cr0;  " :: );

    x64_init_fpu();
    EnableSSE();
    
    return 0;
}


// These functions can be used with GCC (or TCC) 
// to perform some FPU operations without 
// resorting to dedicated assembly.
void fpu_load_control_word(const uint16_t control)
{
    asm volatile ("fldcw %0;"::"m"(control)); 
}


// x64 disable interrupts.
// maybe inline.
void x64_disable_interrupts (void)
{
    asm ("cli"); 
}

// x64 enable interrupts.
void x64_enable_interrupts (void)
{
    asm ("sti");
}


void x64_iretq (void)
{
    asm ("iretq");
}


void x64_iret (void)
{
    asm ("iret");
}


void x64_lret (void)
{
    asm ("lret");
}


void x64_cli_hlt(void)
{
    asm (" cli \n \t "); 
    asm (" hlt \n \t ");
}


void x64_stop_cpu (void)
{
    x64_cli_hlt();
}


// Enable cache.
// credits: barrelfish.
// #todo: Test it on my real machine.

void x64_enable_cache (void)
{
    /*
    uint32_t cr0 = 0; 
    unsigned long cr0 = 0; //use this one ??

    asm volatile ("mov %%cr0, %[cr0]" : [cr0] "=r" (cr0) );

    cr0 &= ~CPUX64_CR0_CD;
    cr0 &= ~CPUX64_CR0_NW;
    
    asm volatile ("mov %[cr0], %%cr0" :: [cr0] "r" (cr0) );
    */
}

// Called by init_architecture_dependent() on core/init.c

int x64_init_intel (void)
{
    //debug_print ("[x64] x64_init_intel:");

    //x64fault_initialize();

    // #bugbug
    // Suspended.
    // We need to teste the effects os this routine in the real machine.
    // See: sysio/hal/arch/x86/x86.h
    // See: sysio/hal/arch/x86/x86.c
    
    //x64_enable_cache();

    // Get info.
    // See: x86.c
    get_cpu_intel_parameters();

    // ...

    return 0;
}


void x64_load_pml4_table(unsigned long phy_addr)
{
    //debug_print("x64_load_pml4_table: DANGER\n");

    asm volatile ("movq %0,%%cr3"::"r"(phy_addr));
}




// If the MP Floating Point Structure 
// can't be found in this area, 
// then the area between 0xF0000 and 0xFFFFF should be searched. 
// See:
// https://wiki.osdev.org/Symmetric_Multiprocessing
void smp_probe(void)
{
    int i=0;

    printf("smp_probe:\n");


// At this point we gotta have a lot of information
// in the structure 'processor'.
    if( (void*) processor == NULL )
        panic("smp_probe: processor\n");

// Is APIC supported.
    if (processor->hasAPIC != TRUE)
        panic("smp_probe: No APIC!\n");


//
// Probe ebda address at bda base.
//

//#define __BDA_BASE    0x040E

    unsigned long ebda_address=0;
    
    // pega um short
    unsigned short *bda = (unsigned short*) 0x040E;

    printf("EBDA short Address: %x \n",bda[0]); 
 
    ebda_address = (unsigned long) ( bda[0] << 4 );
    ebda_address = (unsigned long) ( ebda_address & 0xFFFFFFFF);

    printf("EBDA Address: %x \n",ebda_address); 
    refresh_screen();


//
// Probe 0x5F504D5F signature. 
// "_MP_".
//

#define MP_SIG  0x5F504D5F

    unsigned char *p;

    unsigned char c1;
    unsigned char c2;
    unsigned char c3;
    unsigned char c4;

// base
// between 0xF0000 and 0xFFFFF.
// #todo: filter

    p = ebda_address;

    // the that was found?
    static int mp_found = FALSE;

    int max = (0xFFFFF - ebda_address);
    for(i=0; i<max; i++)
    {
        c1 = p[i];
        c2 = p[i+1];
        c3 = p[i+2];
        c4 = p[i+3];
    
        // "_MP_"
        // This signature is the first element of the table.
        // MP Floating Pointer Structure
        if ( c1 == '_' && c2 == 'M' && c3 == 'P' && c4 == '_' )
        {
            printf (":: Found _MP_ at index %d. :)\n",i);
            mp_found=TRUE;
            break;
        }
    }

    if(mp_found!=TRUE){
        printf("MP table wasn't found!\n");
        refresh_screen();
        return;
    } 

// base + offset.
// This is the base of the structure.
// See:
// basetier/new/include/hal/mp.h

    unsigned long table_address = (ebda_address + i);

    MPTable = (struct mp_floating_pointer_structure *) table_address;

// Print table.


// signature
    printf("Signature: %c %c %c %c\n",
        MPTable->signature[0],
        MPTable->signature[1],
        MPTable->signature[2],
        MPTable->signature[3]);

// configuration table
    //32bit address.
    unsigned long configurationtable_address = 
        (unsigned long) (MPTable->configuration_table & 0xFFFFFFFF);

    printf("Configuration table address %x\n",
        configurationtable_address);

// lenght
// n*16 bytes
    printf("lenght %d \n",MPTable->length);

// revision
// 1.x
    printf("revision %d \n",
        MPTable->mp_specification_revision);

//checksum
    printf("checksum %d \n",MPTable->checksum);
// default configuration flag.
    printf("default_configuration %d \n",MPTable->default_configuration);
//  features.
    printf("features %d \n",MPTable->features);

    if( MPTable->features & (1 << 7) ){
         printf("The IMCR is present and PIC mode is being used\n");
    }

    if( (MPTable->features & (1 << 7)) == 0 ){
         printf("Using the virtual wire mode.\n");
    }

// ==============================================

    MPConfigurationTable = 
        (struct mp_configuration_table *) configurationtable_address;
    
// signature
    printf("Signature: %c %c %c %c\n",
        MPConfigurationTable->signature[0],
        MPConfigurationTable->signature[1],
        MPConfigurationTable->signature[2],
        MPConfigurationTable->signature[3]);

    //int i=0;

    char oemid_string[8+1];
    char productid_string[12+1];

    for(i=0; i<8; i++)
        oemid_string[i] = MPConfigurationTable->oem_id[i];
    oemid_string[8]=0;

    for(i=0; i<12; i++)
        productid_string[i] = MPConfigurationTable->product_id[i];
    productid_string[12]=0;

// intel: OEM00000 PROD00000000
    printf("oem id: %s\n",oemid_string);
    printf("product id: %s\n",productid_string);

    printf("lapic address: %x\n",
        MPConfigurationTable->lapic_address );

// See:
// apic.c
    lapic_initializing( 
        (unsigned long) MPConfigurationTable->lapic_address );


// entries ===================================

// The address of the first entry.
    unsigned long entry_base = 
    (unsigned long) ( configurationtable_address + sizeof( struct mp_configuration_table ) );

    // tmp 
    struct entry_processor *e;

    // How many entries?
    for(i=0; i<4; i++)
    {
        e = (struct entry_processor *) entry_base;
        
        if(e->type != 0)
        {
            printf("#test: Not a processor entry\n");
            refresh_screen();
            return;
        }

        printf("local_apic_id %d\n",e->local_apic_id);
        printf("local_apic_version %d\n",e->local_apic_version);

        if( (e->flags & (1<<0)) == 0 ){
            printf("Processor must be ignored\n");
        }

        if( e->flags & (1<<1) ){
            printf("The processor is a bootstrap processor\n");
        }
        
        printf ("stepping: %d\n",(e->signature & 0x00F));
        printf ("   model: %d\n",((e->signature & 0x0F0) >> 4) );
        printf ("  family: %d\n",((e->signature & 0xF00) >> 8) );

        // Next entry
        entry_base = (unsigned long) (entry_base + 20);
    };

    printf("smp_probe: done\n");
    refresh_screen();
}





