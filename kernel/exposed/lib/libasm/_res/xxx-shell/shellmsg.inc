;
; File: shell\shellmsg.inc 
;
; Descri??o:
;     Mensagens e vari?veis do Shell do Boot Manager.
;
; Hist?rico:
;     2015 - Created.
;     2017 - Revision.
;


;
; Vari?veis:
; ==========
;

;				
; Struct.
;     Estrtura de strings aleat?rias para suporte ao shell do boot manager. 
;

SHELL:
   .volume:    db 0         ;Em que volume est? o Shell.
   .directory: db 0         ;Em que diret?rio est? o Shell.
   .separador1: db '>',0    ;Separador1.
   ;.separador2: db '>>',0    ;Separador2.
   ;.money: db '$',0
   ;.slash: db '\\', 0 
   ;...  
  
;;...

   

;Prompt:   
;Endere?o da string no novo prompt.   
new_prompt:  dd 0              
 
   

;Flag:
;Se vai mudar o nome de um arquivo.
;O que fazer com o arquivo? ... Salvar, Abrir.
file_flag db 0       
   
   

;
; Command strings.
;

;
; @todo: 
;     Incluir configura??es de inicializa??o, 
;     como as do menu16.
;



;Inicializa??o.
boot_string		  db     'boot', 0    ;Inicia com a configura??o atual. 
reboot_string	  db   'reboot', 0    ;Reinicia o sistema.
shutdown_string   db 'shutdown', 0    ;Desliga a m?quina via APM.	
make_boot_string  db 'makeboot', 0    ;Cria inicializa??o padr?o. 
;...

;Opera??es com dispositivos.
hd_string             db        'disk', 0    ;Visualiza as informa???es sobre um disco.  
particao_string       db        'part', 0    ;Visualiza as informa??es sobre uma parti??o
;;..

;Opera??es com arquivos.
dir_string		    db 'dir', 0    ;Exibe a lista de arquivos do diret?rio raiz.
open_file_string    db 'of', 0     ;Open file.
make_file_string    db 'mf', 0     ;Make file.
delete_file_string  db 'df', 0     ;Delete file.
open_dir_string     db 'od', 0     ;Open dir.
make_dir_string     db 'md', 0     ;Make dir.
delete_dir_string   db 'dd', 0     ;Delete dir.

change_dir_string   db 'cd', 0     ;Change dir. (Change current working dir.)
print_workingdir_string   db 'pwd', 0     ;pwd (print working directory) 
ls_string                 db 'ls', 0     ;ls lista os arquivos.
;...

;Outros.
help_string		      db        'help', 0
cls_string		      db         'cls', 0
ver_string		      db         'ver', 0
exit_string		      db        'exit', 0
debug_string	      db       'debug', 0
testzone_string       db    'testzone', 0    ;; Efetuando o teste atual.
install_string        db     'install', 0    ;Transfere arq.
mount_volume_string   db       'mount', 0    ;Monta o volume do sistema se poss?vel.
format_disk_string    db      'format', 0    ;Formata uma parti??o selecionado.
activate_partition    db    'activate', 0    ;Ativa uma parti??o e/ou disco selecionados.
deactivate_partition  db  'deactivate', 0    ;Ativa uma parti??o e/ou disco selecionados.
save_partition        db        'save', 0    ;Torna permanente a sele??o.
;...

;Ativa novo modo de inicializa??o.
modo1_string  db 'mode1', 0
modo2_string  db 'mode2', 0
modo3_string  db 'mode3', 0
modo4_string  db 'mode4', 0
;...


;Disks. (Vari?vel de ambiente.)
d0_string  db '/disk0', 0
d1_string  db '/disk1', 0
d2_string  db '/disk2', 0
d3_string  db '/disk3', 0

;Volumes. (Vari?vel de ambiente.)
v0_string  db '/volume0', 0
v1_string  db '/volume1', 0
v2_string  db '/volume2', 0
v3_string  db '/volume3', 0

;Selecionadores.
seleciona_d0  db  'd0', 0    ;Seleciona disco 0.
seleciona_d1  db  'd1', 0    ;Seleciona disco 1.
seleciona_d2  db  'd2', 0    ;Seleciona disco 2.
seleciona_d3  db  'd3', 0    ;Seleciona disco 3.
seleciona_p0  db  'p0', 0    ;Seleciona particao 0.
seleciona_p1  db  'p1', 0    ;Seleciona particao 1.
seleciona_p2  db  'p2', 0    ;Seleciona particao 2.
seleciona_p3  db  'p3', 0    ;Seleciona particao 3.
;...



;=====================================================================
; * Strings *
;
; Principais string e mensagens do Shell do Boot Manager.
;
;

;Splash.
shellmsg_splash: 
    db 'The Gramado Boot Manager Shell',13,10, 
    db 'Version ', BOOTMANAGER_VER, ' (C) Copyright Fred Nora 2017',13,10,0


;Prompt.
prompt:              db 'BM/>', 0    ;Prompt padr?o.
msg_shell_prompt:    db 'BM/>', 0
prompt_change_file:  db 'filename=', 0      ;Para declarar o nome do arquivo.
;...

;Volume support.
system_volume_string db 'VOLUME0 VOL',0
;...


;File support.
appextension:	     db '.app', 0
dirextension:	     db '.dir', 0

;C?pia do metafile que ser? salva no sistema de arquivos.
metafile1_string:        db 'metafilemf1',0     ;
metafile1_upper_string:  db 'METAFILEMF1',0     ;
metafile1_fullname:      db 'METAFILE.MF1',0    ;

;Arquivo BL.BIN.
file1_string:        db 'bl      bin',0     ;File name.
file1_upper_string:  db 'BL      BIN',0     ;File name.
file1_fullname:      db 'BL.BIN',0


;Diret?rio /boot.
dir1_string:         db 'boot    dir',0
dir1_upper_string:   db 'BOOT    DIR',0
dir1_fullname:       db 'BOOT.DIR',0
dir1_slash_string:         db '/boot',0
dir1_upper_slash_string:   db '/BOOT',0
;...


; File operation.
msg_file_op0 db "Default operation on file", 13, 10 ,0
msg_file_op1 db "Open file", 13, 10 ,0
msg_file_op2 db "Create file", 13, 10 ,0
msg_file_op3 db "Delete file", 13, 10 ,0
msg_file_op4 db "Open dir", 13, 10 ,0
msg_file_op5 db "Create dir", 13, 10 ,0
msg_file_op6 db "Delete dir", 13, 10 ,0
msg_file_op7 db "Open app", 13, 10 ,0
;...


;Strings e messages.   
version_msg:		db 'Gramado Boot Manager Version ', BOOT_VER, 
                    db ' (c) Copyright 2013-2018', 13, 10, 0

not_found_msg:	    db 'Command or file not found', 13, 10, 0
hd_msg:             db 'Reading sector', 13, 10, 0
dir_title1_string:  db "Current Directory:", 13, 10, 0
dir_title2_string:  db "Name                     Cluster", 13, \
                       "================================", 13, 0

;
; Help text.
;


help_text:
    db '[INITIALIZATION:]', 13, 10
	db 'boot, reboot, shutdown', 13, 10
    db '                     ', 13, 10
    db '[DEVICES:]', 13, 10
    db 'disk, part', 13, 10
    db '                     ', 13, 10
	db '[FILE:]', 13, 10
	db 'dir', 13, 10
    db '                     ', 13, 10
	db '[MORE:]', 13, 10
    db 'help, cls, ver', 13, 10, 0


;
;fim.
;

