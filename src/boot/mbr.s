%include"boot.inc"
section text vstart=0x7c00
global _start
_start:
	mov ax,cs
	mov ss,ax
	mov ds,ax
	mov fs,ax
	mov gs,ax
	mov es,ax
	mov sp,0x7c00
	mov ax,0xb800
	mov es,ax
	
	mov ax,0x0600
	mov bx,0x0700
	mov cx,0
	mov dx,0x184f
	int 0x10

	mov si,load_loader_str
	mov di,0
	mov cx,load_loader_str_end-load_loader_str
print_load_process:
	mov al,[ds:si]
	mov [es:di],al
	mov byte [es:di+1],0b0111_1000
	inc si
	add di,2
	loop print_load_process

	call read_loader
	
	jmp 0x0000:0x0900
read_loader:
	mov bx,LOADER_START_SECTOR	;2
	mov ax,LOADER_BASE_ADDR		;0x900
	push ax
	push bx
	
	;set the count of reading sector 
	mov cx,4
	mov dx,0x1f2
	mov ax,cx
	out dx,al
	
	;set LBA address
	mov ax,bx
	mov dx,0x1f3
	out dx,al
	
	mov dx,0x1f4
	shr ax,8
	out dx,al	
	
	mov dx,0x1f5
	shr ax,8
	out dx,al
	
	mov dx,0x1f6
	shr ax,8
	and al,0x0f
	or al,0xe0
	out dx,al
	
	mov dx,0x1f7
	mov al,0x20
	out dx,al
	
	mov dx,0x1f7
not_ready:
	in al,dx
	and al,0x08
	cmp al,0x08
	jne not_ready
	pop bx
	pop ax
	
	mov di,ax
	mov cx,1024
	mov dx,0x1f0
read:
	in ax,dx
	mov [di],ax
	add di,2
	loop read
	ret
load_loader_str:
	db "loading loader ...",0
load_loader_str_end:	

	times 510-($-$$) db 0
	db 0x55,0xaa
