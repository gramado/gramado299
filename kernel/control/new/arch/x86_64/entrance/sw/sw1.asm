
; sw.asm
; This file handles syscall for x86_64 processors.
; 0x80, 0x81, 0x82

align 16
__sw_local_fpu_buffer:
    times 512 db 0
align 16


;------------------------
; _int128
;     System Call number 0x80
;     >>>> ONLY CALLED FROM USERMODE!
;     + It is never called from kernel mode.
;     + It calls a system service without changing the segment registers.
;     + We are using the caller cr3.
;     It has four parameters:
;     rax - Argument 1
;     rbx - Argument 2
;     rcx - Argument 3
;     rdx - Argument 4
;     #todo: 
;     Maybe we can receive more values using more registers.

extern _sci0
extern _sci0_cpl

; Capture context
global _int128
_int128:

; #todo: Maybe we can save the stack frame.

    pop qword [.int128_rip]
    pop qword [.int128_cs]

    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    
    ;push ds
    ;push es
    push fs
    push gs
    push rsp
    pushfq

    ; Parameters:
    ; RDI, RSI, RDX, RCX, R8, and R9 are used 
    ; for integer and memory address arguments

    mov rdi, rax  ; arg1: service number
    mov rsi, rbx  ; arg2:
    push rdx      ; Saving arg4
    mov rdx, rcx  ; arg3:
    pop rcx       ; arg4: 
  

; cpl
; Get the first 2 bits of cs.
; see: sci.c
; #importante: temos que pegar cs na pilha e nao no registrador.
; talvez durante a systemcall o processador mude o cs para o valor 
; do seletor indicado na entrada da idt.
    xor rax, rax
    mov rax, qword [.int128_cs]
    and rax, 3
    mov qword [_sci0_cpl], rax


    fxsave [__sw_local_fpu_buffer]

    call _sci0

    fxrstor [__sw_local_fpu_buffer]

    mov qword [.int128Ret], rax 

    popfq
    pop rsp
    pop gs
    pop fs
    ;pop es
    ;pop ds

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    mov rax, qword [.int128Ret] 

    push qword [.int128_cs]      ; cs
    push qword [.int128_rip]     ; rip
    iretq
.int128Ret: dq 0
.int128_cs: dq 0
.int128_rip: dq 0
;--  






;;-----
; _int129
; 
;     System Call number 0x81
;
;     ONLY CALLED FROM USERMODE!
;
;     + It is never called from kernel mode.
;     + It chamges the segment registers before calling the system service.
;     + We are using the caller cr3.
;
;     It has four parameters:
;     rax - Argument 1
;     rbx - Argument 2
;     rcx - Argument 3
;     rdx - Argument 4
;
;     #todo: 
;     Maybe we can receive more values using more registers.
;
;;-----

;extern _xxxxINT129_DEBUG_MESSAGE
extern _sci1
extern _sci1_cpl
; Capture context
global _int129
_int129:

; #todo: Maybe we can save the stack frame.

    pop qword [.int129_rip]
    pop qword [.int129_cs]

    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    
    ;push ds
    ;push es
    push fs
    push gs
    push rsp
    pushfq

    ; Parameters:
    ; RDI, RSI, RDX, RCX, R8, and R9 are used 
    ; for integer and memory address arguments

    mov rdi, rax  ; arg1: service number
    mov rsi, rbx  ; arg2:
    push rdx      ; Saving arg4
    mov rdx, rcx  ; arg3:
    pop rcx       ; arg4: 


; cpl
; Get the first 2 bits of cs.
; see: sci.c
; #importante: temos que pegar cs na pilha e nao no registrador.
; talvez durante a systemcall o processador mude o cs para o valor 
; do seletor indicado na entrada da idt.
    xor rax, rax
    mov rax, qword [.int129_cs]
    and rax, 3
    mov qword [_sci1_cpl], rax


    fxsave [__sw_local_fpu_buffer]

    call _sci1

    fxrstor [__sw_local_fpu_buffer]


    mov qword [.int129Ret], rax 

    popfq
    pop rsp
    pop gs
    pop fs
    ;pop es
    ;pop ds

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    mov rax, qword [.int129Ret] 

    push qword [.int129_cs]      ; cs
    push qword [.int129_rip]     ; rip
    iretq
.int129Ret: dq 0
.int129_cs: dq 0
.int129_rip: dq 0
;--  




;;-----
; _int128
; 
;     System Call number 0x82
;
;     ONLY CALLED FROM USERMODE!
;
;     + It is never called from kernel mode.
;     + It chamges the segment registers before calling the system service.
;     + We are using the caller cr3.
;
;     It has four parameters:
;     rax - Argument 1
;     rbx - Argument 2
;     rcx - Argument 3
;     rdx - Argument 4
;
;     #todo: 
;     Maybe we can receive more values using more registers.
;
;;-----

;extern _xxxxINT130_DEBUG_MESSAGE
extern _sci2
extern _sci2_cpl
; Capture context
global _int130
_int130:

; #todo: Maybe we can save the stack frame.

    pop qword [.int130_rip]
    pop qword [.int130_cs]

    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    
    ;push ds
    ;push es
    push fs
    push gs
    push rsp
    pushfq

    ; Parameters:
    ; RDI, RSI, RDX, RCX, R8, and R9 are used 
    ; for integer and memory address arguments

    mov rdi, rax  ; arg1: service number
    mov rsi, rbx  ; arg2:
    push rdx      ; Saving arg4
    mov rdx, rcx  ; arg3:
    pop rcx       ; arg4: 


; cpl
; Get the first 2 bits of cs.
; see: sci.c
; #importante: temos que pegar cs na pilha e nao no registrador.
; talvez durante a systemcall o processador mude o cs para o valor 
; do seletor indicado na entrada da idt.
    xor rax, rax
    mov rax, qword [.int130_cs]
    and rax, 3
    mov qword [_sci2_cpl], rax


    fxsave [__sw_local_fpu_buffer]

    call _sci2

    fxrstor [__sw_local_fpu_buffer]


    mov qword [.int130Ret], rax 

    popfq
    pop rsp
    pop gs
    pop fs
    ;pop es
    ;pop ds

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    mov rax, qword [.int130Ret] 

    push qword [.int130_cs]      ; cs
    push qword [.int130_rip]     ; rip
    iretq
.int130Ret: dq 0
.int130_cs: dq 0
.int130_rip: dq 0
;--    


; int 198
; Restorer.
; Used to return from ring 3 to ring0
; when the kernel calls a procedure in ring 3.
; It can be a signal or a window procedure.
; The purpose here is jumping to a c routine,
; restore the context with the idle thread
; and restart the ifle thread.
;extern _x64_restorer
;global _int198
;_int198:
;    jmp _x64_restorer
    
    



; int 199
; This is called only from the init process in user mode.
; It will enable the interrupts and the taskswitching
; for the first time.
; see: INIT.BIN.

global _int199
_int199:
    pop qword [.frameRIP]
    pop qword [.frameCS]
    pop qword [.frameRFLAGS]

; iopl 3
    ;mov qword [.frameRFLAGS], 0x0000000000003200
; iopl 0
    mov qword [.frameRFLAGS], 0x0000000000000200
    
    push qword [.frameRFLAGS]
    push qword [.frameCS]
    push qword [.frameRIP]
    iretq
.frameRIP:     dq 0
.frameCS:      dq 0
.frameRFLAGS:  dq 0







