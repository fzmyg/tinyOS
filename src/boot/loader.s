%include"boot.inc"
[bits 16]
	jmp _start
GDT:
        GDT_START: dq 0

        GDT_CODE_ENTRY: dd  0x0000ffff
                        dd  DESC_CODE_HIGH4

        GDT_DATA_ENTRY: dd 0x0000ffff
                        dd DESC_DATA_HIGH4

        GDT_VIDEO_ENTRY: dd 0x80000008
                        dd DESC_VIDEO_HIGH4

        REVERSED_ENTRY: times 60  dq 0
GDT_END:

total_mem_bytes_size: dw 0
ards_buf: times 244 db 0
ards_num: dw 0

section .text vstart=LOADER_BASE_ADDR
[bits 16]
_start:
	mov sp,LOADER_BASE_ADDR
	
	mov eax,0xe820
	xor ebx,ebx
	mov edi,ards_buf
	mov edx,0x534d4150
	mov ecx,20
	call .e820_get_mem
	jmp .memory_get_ok
.e820_get_mem:	
	add esi,20
	inc word [ards_num]
	int 0x15
	jc .e801_get_mem
	cmp ebx,0
	jne .e820_get_mem
	
	mov ebx,ards_buf
	xor edx,edx
	mov cx,[ards_num]
.find_max_memory_capa:
	mov eax,[ebx]
	add eax,[ebx+8]
	cmp eax,edx
	jng  .next_ards
	mov eax,edx
.next_ards:
	loop .find_max_memory_capa
	ret
.e801_get_mem:
	mov eax,0xe801
	int 0x15
	jc .try88
	xor edx,edx
	imul eax,1024
	imul ebx,1024*64
	add eax,ebx	
	mov edx,eax
	add edx,1024*1024
	ret
.try88:
	mov ah,0x88
	int 0x15
	jc .err_hlt
	imul eax,1024
	add eax,1024*1024
	ret
.err_hlt:
	hlt
.memory_get_ok:
	mov [total_mem_bytes_size],edx

	
	;open a20
	in al,0x92
	or al,0b0000_0010	
	out 0x92,al

	;load gdt
	lgdt [gdt_ptr]

	;power on the pe port
	mov eax,cr0
	or eax,0b0000_0001
	mov cr0,eax
	jmp dword SELECTOR_CODE:p_mode_start
[bits 32]
p_mode_start:
	mov ax,SELECTOR_DATA
	mov ds,ax
	mov ss,ax	
	mov es,ax	
	mov esp,LOADER_STACK_TOP
	mov si,msg	
	mov di,0
	mov ecx,msg_end-msg
	call print
	call setup_page
	
	mov eax,PAGE_DIR_TABLE_ADDR			;set pdt physical address
	mov cr3,eax					
	mov eax,cr0
	or  eax,0x80000000
	mov cr0,eax
	
	sgdt [gdt_ptr]
	mov ebx,[gdt_ptr+2]
	or dword [ebx+0x18+0x4],0xc0000000		;set video desctor to virtual address	
	;or dword [ebx+0x10+0x4],0xc0000000		;set data  desctor to virtual address
	;or dword [ebx+0x08+0x4],0xc0000000		;set code  desctor to virtual address
	or dword [gdt_ptr+2],0xc0000000			;set gdt address to virtual address
	
	lgdt [gdt_ptr]
	or esp,0xc0000000
	call read_kernel
	call init_kernel
	mov esp,0xc009f000		;保留区域最高位
	jmp  SELECTOR_CODE:KERNEL_ENTER_POINT 

print:
	mov ax,SELECTOR_VIDEO
	mov gs,ax
	mov byte al,[ds:si]
	mov byte [gs:di],al
	mov byte [gs:di+1],0x07
	inc si
	add di,2
	loop print
	ret


setup_page:
	mov ecx,4096
	mov esi,0
	clear_page_dir_mem:
		mov byte [PAGE_DIR_TABLE_ADDR+esi],0	
		inc esi
		loop clear_page_dir_mem
	create_pde:
		mov eax,PAGE_DIR_TABLE_ADDR
		add eax,0x1000
		or eax, PG_US_U | PG_WR_W | PG_P
		mov [PAGE_DIR_TABLE_ADDR+0x0],eax   ; the first dir ---> the first page
		mov [PAGE_DIR_TABLE_ADDR+768*4],eax ; the 769th dir ---> the first page
		sub eax,0x1000
		mov [PAGE_DIR_TABLE_ADDR+1023*4],eax ; the last dir ---> the first dir
	
	;create the first pte    the behind 256 entries map the physical memory address 0x00000---0xfffff cap:1MB
		mov ebx,PAGE_DIR_TABLE_ADDR+0x1000
		mov ecx,256
		xor esi,esi
		xor edx,edx
		mov edx,PG_US_U|PG_WR_W|PG_P
	create_pte:
		mov [ebx+esi*4],edx
		add edx,4096
		inc esi
		loop create_pte
	
		mov eax,PAGE_DIR_TABLE_ADDR+0x2000
		mov ecx,254
		mov esi,769
		mov ebx,PAGE_DIR_TABLE_ADDR
		or eax,PG_US_U|PG_WR_W|PG_P
	create_kernel_pde:
		mov [ebx+esi*4],eax
		inc esi
		add eax,0x1000
		loop create_kernel_pde
		ret	

read_kernel:
	mov eax,KERNEL_SECTOR_CNT
	mov dx,0x1f2
	out dx,al
	
	mov dx,0x1f3
	mov eax,KERNEL_LBA_ADDR
	out dx,al
	
	mov dx,0x1f4
	shr eax,8
	out dx,al
	
	mov dx,0x1f5
	shr eax,8
	out dx,al
	
	mov dx,0x1f6
	shr eax,8
	or al,0xe0
	out dx,al
	
	mov dx,0x1f7
	mov al,0x20
	out dx,al
	
	.wait_for_disk:
		nop
		in al,dx
		and al,0x88
		cmp al,0x80
		je .wait_for_disk
	mov ecx,102400
	mov edi,KERNEL_ELF_BASE_ADDR
	mov dx,0x1f0
	.set2mem:
		in ax,dx
		mov [ds:edi],ax
		add edi,2
		loop .set2mem
	ret
;void mem_cpy(dest ,src ,size)
mem_cpy:
	cld
	push ebp
	mov ebp,esp
	pushad
	mov edi,[ebp+8]
	mov esi,[ebp+12]
	mov ecx,[ebp+16]
	cmp ecx,0
	je .mem_cpy_done
	.mem_cpy:
	mov al,[esi]
	mov [edi],al
	inc esi
	inc edi
	loop .mem_cpy
	.mem_cpy_done:
	popad
	mov esp,ebp
	pop ebp
	ret

init_kernel:
	xor eax,eax	;
	xor ebx,ebx	;programer header offset
	xor ecx,ecx	;programer header num
	xor edx,edx	;each programer  header size
	
	mov dx,[KERNEL_ELF_BASE_ADDR+42]		;each programer header size
	mov cx,[KERNEL_ELF_BASE_ADDR+44]		;count of programer header
	mov ebx,[KERNEL_ELF_BASE_ADDR+28]		;first programer offset
	add ebx,KERNEL_ELF_BASE_ADDR		
	.read_each_p_header:
		cmp byte [ebx+0],PT_NULL
		je .PTNULL
		push dword [ebx+16]		;size
		mov eax,[ebx+4]
		add eax,KERNEL_ELF_BASE_ADDR
		push eax			;si
		mov eax,dword [ebx+8]		;di
		and eax,0x3fffffff
		push eax
		call mem_cpy			;void mem_cpy(di,si,size);
		add esp,12
	.PTNULL:
		add ebx,edx
		loop .read_each_p_header 
		ret
GDT_SIZE equ $-GDT_START

GDT_LIMITS equ GDT_SIZE-1

KERNEL_SECTOR_CNT equ 400

gdt_ptr: dw GDT_LIMITS	
	 dd GDT_START

LOADER_STACK_TOP equ LOADER_BASE_ADDR

msg:
	db "loader is running!",0
msg_end:
load_loader_msg:
	db "loading loader...",0
load_loader_msg_end:
	
