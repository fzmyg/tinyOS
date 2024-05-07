VIDEO_SELECTOR equ (3<<3)+TI+RPL0
TI 	equ 0x000
RPL0 	equ 0x00
section .text
; void put_char(int ch);   eax = ch
global put_char
put_char:
	push ebp
	mov ebp,esp
	pushad
	
	xor eax,eax
	mov ax,VIDEO_SELECTOR
	mov gs,ax
	
	mov dx,0x3d4
	mov al,0xe
	out dx,al
	mov dx,0x3d5
	in al,dx
	shl eax,8	
	mov dx,0x3d4
	mov al,0x0f
	out dx,al
	mov dx,0x3d5
	in al,dx
	and eax,0x0000ffff
	mov ebx,eax		;ebx cursor location
	mov eax,[ebp+8]		;eax int ch
	
	cmp al,0xd  ;enter
	je .is_enter
	cmp al,0xa  ;new line
	je .is_newline
	cmp al,0x8  ;backspace
	je .is_backspace
	cmp al,0x09 ;tab
	je .is_tab
	jmp .is_other_chara
.is_enter:
.is_newline:
	;ebx : loc
	mov eax,ebx
	xor edx,edx
	mov ecx,80
	div ecx
	inc eax
	mov ecx,80
	mul ecx
	cmp eax,2000
	jae .roll_screen	
	mov ebx,eax
	jmp .set_cursor
	
.is_backspace:
	cmp ebx,0
	je .put_char_done
	dec ebx
	mov byte [gs:ebx+ebx],' '
	;dec ebx
	jmp .set_cursor
.is_tab:
	mov edi,2000
	mov eax,edi
	sub eax,ebx
	mov ecx,eax
	add ecx,ecx
	mov edi,4000
	mov esi,3992
	.move:
		mov byte al,[gs:esi]	
		mov byte [gs:edi],al
		dec edi
		dec esi
		loop .move
	add ebx,4
	mov dword [gs:ebx+ebx-4],0
	mov dword [gs:ebx+ebx-8],0
	jmp .set_cursor

.is_other_chara:
	mov [gs:ebx+ebx],al	
	mov byte [gs:ebx+ebx+1],0x07
	inc ebx
	jmp .set_cursor

.roll_screen:
	mov esi,160
	mov edi,0	
	mov ecx,1920*2
	.move_each_byte:
	mov  al,[gs:esi]
	mov  [gs:edi],al
	inc edi
	inc esi
	loop .move_each_byte	
	mov esi,1920*2
	mov ecx,160
	.clean_last_line:
	mov byte [gs:esi],0
	inc esi
	loop .clean_last_line
	mov ebx,1920
	jmp .continue_set_cursor
.set_cursor:
	cmp ebx,2000
	jae .roll_screen
	.continue_set_cursor:	
	mov dx,0x3d4
	mov ax,0x0f
	out dx,ax
	mov dx,0x3d5
	mov al,bl
	out dx,al
	shr ebx,8
	mov dx,0x3d4
	mov ax,0x0e
	out dx,ax	
	mov dx,0x3d5
	mov al,bl
	out dx,al
	jmp .put_char_done
.put_char_done:
	popad
	mov esp,ebp
	pop ebp
	ret




global cls
cls:
	push ebp
	mov ebp,esp
	pushad
	mov ax,(3<<3)+0+0
	mov gs,ax
	mov esi,0
	mov ecx,2000
	.clean:
	mov byte [gs:esi],0x20
	mov byte [gs:esi+1],0x07
	add esi,2
	loop .clean
	mov ebx,0
	mov dx,0x3d4
	mov al,0x0e
	out dx,al
	mov dx,0x3d5
	mov al,0
	out dx,al
	mov dx,0x3d4
	mov al,0x0f
	out dx,al
	mov dx,0x3d5
	mov al,0
	out dx,al
	popad
	mov esp,ebp
	pop ebp
	ret 


global put_str
put_str:
	push ebp
	mov ebp,esp
	pushad
	mov ebx,[ebp+8]
	xor eax,eax
.read_each_char:
	mov byte al , [ebx]	
	cmp al,0
	je .put_str_done
	push eax
	call put_char
	add esp,4
	inc ebx
	jmp .read_each_char		

.put_str_done:
	popad
	mov esp,ebp
	pop ebp
	ret

global put_int
put_int:
	push ebp
	mov ebp,esp	
	pushad
	mov eax,[ebp+8]
	mov ecx,8
	xor ebx,ebx
	;mov esi,put_int_buf_end-1-0xc0000000
	mov esi,put_int_buf_end-2
.put_each_num:
	mov bl,al
	and bl,0x0f
	cmp bl,0x0a
	jl .put_number
.put_alph:	 	
	add bl,'a'-10
	jmp .next_process
.put_number:
	add bl,'0'
.next_process:
	mov  byte [esi],bl
	dec esi
	shr eax,4
	loop .put_each_num
	mov byte [esi],'x'	
	mov byte [esi-1],'0'
	push put_int_buf
	call put_str
	add esp,4
	
	popad
	mov esp,ebp
	pop ebp
	ret
;extern void setcursor(int);
global set_cursor
set_cursor:
	push ebp
	mov ebp,esp
	pushad
	mov ebx,[ebp+8]
        mov dx,0x3d4
        mov ax,0x0f
        out dx,ax
        mov dx,0x3d5
        mov al,bl
        out dx,al
        shr ebx,8
        mov dx,0x3d4
        mov ax,0x0e
        out dx,ax
        mov dx,0x3d5
        mov al,bl
        out dx,al	
	popad
	mov esp,ebp
	pop ebp
	ret
global cursor_up
cursor_up:
	push ebp
	mov ebp,esp
	pushad
	mov dx,0x3d4
        mov al,0xe
        out dx,al
        mov dx,0x3d5
        in al,dx
        shl eax,8
        mov dx,0x3d4
        mov al,0x0f
        out dx,al
        mov dx,0x3d5
        in al,dx
        and eax,0x0000ffff
        mov ebx,eax             ;ebx cursor location
	
	sub ebx,80
	cmp ebx,0
	jl .cursor_up_end
	push ebx
	call set_cursor
	add esp,4

.cursor_up_end:
	popad
	mov esp,ebp
	pop ebp
	ret
global cursor_down
cursor_down:
        push ebp
        mov ebp,esp
        pushad
        mov dx,0x3d4
        mov al,0xe
        out dx,al
        mov dx,0x3d5
        in al,dx
        shl eax,8
        mov dx,0x3d4
        mov al,0x0f
        out dx,al
        mov dx,0x3d5
        in al,dx
        and eax,0x0000ffff
        mov ebx,eax             ;ebx cursor location

        add ebx,80
        cmp ebx,2000
        jnl .cursor_up_end
        push ebx
        call set_cursor
        add esp,4

.cursor_up_end:
        popad
        mov esp,ebp
        pop ebp
        ret
	
global cursor_left
cursor_left:
        push ebp
        mov ebp,esp
        pushad
        mov dx,0x3d4
        mov al,0xe
        out dx,al
        mov dx,0x3d5
        in al,dx
        shl eax,8
        mov dx,0x3d4
        mov al,0x0f
        out dx,al
        mov dx,0x3d5
        in al,dx
        and eax,0x0000ffff
        mov ebx,eax             ;ebx cursor location

        sub ebx,1
        cmp ebx,0
        jl .cursor_up_end
        push ebx
        call set_cursor
        add esp,4

.cursor_up_end:
        popad
        mov esp,ebp
        pop ebp
        ret

global cursor_right
cursor_right:
        push ebp
        mov ebp,esp
        pushad
        mov dx,0x3d4
        mov al,0xe
        out dx,al
        mov dx,0x3d5
        in al,dx
        shl eax,8
        mov dx,0x3d4
        mov al,0x0f
        out dx,al
        mov dx,0x3d5
        in al,dx
        and eax,0x0000ffff
        mov ebx,eax             ;ebx cursor location

        add ebx,1
        cmp ebx,2000
        jnl .cursor_up_end
        push ebx
        call set_cursor
        add esp,4

.cursor_up_end:
        popad
        mov esp,ebp
        pop ebp
        ret

section .data
put_int_buf: times 11 db 0
put_int_buf_end:
