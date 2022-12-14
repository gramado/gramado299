;
; File: hardware.inc
;
; Descri??o:
;     +Rotinas de hardware;
;     +Armazenar vari?veis de informa??es sobre o hardware.
;      Disco, CPU, outros ...
;
; Vers?o: 1.0, 2015, 2106.
;



;--------------------------------------------------
; Disk support:
; ============
;
; Qual disco est? selecionado e 
; qual ? a sua parti??o ativa. 
;
; Obs: Superte ? 4 discos por enquanto.
;
DiskBlock:
.number: db 0    ;Qual o n?mero do disco selecionado.
.part:   db 0    ;Qual o n?mero da parti??o selecionada.

;...


Disk0:
.status: db 0         
.particao_ativa: db 0
;...

Disk1:
.status: db 0
.particao_ativa: db 0
;...

Disk2:
.status: db 0
.particao_ativa: db 0
;...

Disk3:
.status: db 0
.particao_ativa: db 0
;...


;
; CPU support.
;

CPUBlock:
.number: db 0    ;Qual o n?mero do cpu selecionado.
;...



;
; CurrentDisk.xxxx
;parametros do disco atual
;	
CurrentDisk:	
.disk_number       dd 0 ;n?mero do disco selecionado.
.particao_ativa    dd 0 ;n?mero da parti??o ativa.
.partition_number  dd 0 ;n?mero da parti??o selecionada.
.flag_p0  db 0
.flag_p1  db 0
.flag_p2  db 0
.flag_p3  db 0	
	

;
; CurrentPartition.xxxx
;parametros da parti??o atual.
;
CurrentPartition:
;...
.flag:  db 0
;...
.h_inicial: db 0      
.c_inicial: db 0      
.s_inicial: db 0
;...	
.os_type:  db 0
;...
.h_final:  db 0
.c_final:  db 0
.s_final:  db 0
;...
.lba_inicial:  db 0x00, 0x00, 0x00, 0x00 
.tamanho_da_particao:  db 0x00, 0x00, 0x00, 0x00 

	
;
; ParticaoAtual.xxxx
;    Configura??es permanentes 
;    da PARTI??O ATUAL.
;    (current partition)
;	
ParticaoAtual:
;...
.flag:      db 0
;...
.h_inicial: db 0      
.c_inicial: db 0      
.s_inicial: db 0
;...	
.os_type:   db 0
;...
.h_final:   db 0
.c_final:   db 0
.s_final:   db 0
;...
.lba_inicial:  db 0x00, 0x00, 0x00, 0x00 
.tamanho_da_particao:  db 0x00, 0x00, 0x00, 0x00 

	
;
; parti??o 0 - (8MB)
;
;vbr  - ?  ;134  (depende do tanto de setores reservados.)
;fat1 - 136
;fat2 - 200
;root - 264
;data - 296
;

;
; first_partition.xxxx	
;primeira parti??o - estrutura.
first_partition:
.mbr   dd 0
.vbr   dd 134 
.fat1  dd 136
.fat2  dd 200
.root  dd 264
.data_area  dd 296

	
	
;parti??o ativa
g_particao_ativa:     db 0
g_particao_ativa_lba: dd 0



;;======================================================
; hardware_show_currentdisk_info:
;     Mostra par?metros do disco atual.
;
;
hardware_show_currentdisk_info:
    pushad

    mov esi, msg_disk_Starting
	call os_output		 	

	
	;Disk Number
    mov esi, msg_disk_DiskNumber
	call os_output		 	
	xor eax, eax
	mov al, BYTE [META$FILE.DISK_NUMBER]
    call print_hex	
	
	;Heads.
    mov esi, msg_disk_Heads
	call os_output		 	
	xor eax, eax
	mov ax, word [META$FILE.HEADS]
    call print_hex	

	;SPT.
    mov esi, msg_disk_SPT
	call os_output		 	
	xor eax, eax
	mov al, byte [META$FILE.SPT]
    call print_hex	
		

	;CYLINDERS.
    mov esi, msg_disk_Cylinders
	call os_output		 	
	xor eax, eax
	mov ax, word [META$FILE.CYLINDERS]
    call print_hex	
	
	
	;Continua ...
	
    call os_print_newline	
	
	popad
    ret


msg_disk_Starting   db "[Disk Info:]", 13, 10 ,0	
msg_disk_DiskNumber db "DiskNumber=",0
msg_disk_Heads      db " Heads=",0
msg_disk_SPT        db " Spt=",0
msg_disk_Cylinders  db " Cylinders=",0

;
;fim.
;
