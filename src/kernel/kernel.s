%define ERROR_CODE nop
%define ZERO push 0
%macro INT_VECTOR 2
section .text
int_%1_entry:
	%2
	push ds
	push es
	push fs
	push gs
	pushad
	mov al,0x20
	out 0x20,al
	out 0xa0,al
	push %1
	call [int_vector_table+%1*4]
	jmp int_exit

section .data
	dd int_%1_entry
%endmacro


extern put_str

section .data
extern int_vector_table
int_str: db "int occur!",0xa,0
global int_entry_table
int_entry_table:
	INT_VECTOR 0x00,ZERO
	INT_VECTOR 0x01,ZERO
	INT_VECTOR 0x02,ZERO
	INT_VECTOR 0x03,ZERO
	INT_VECTOR 0x04,ZERO
	INT_VECTOR 0x05,ZERO
	INT_VECTOR 0x06,ZERO
	INT_VECTOR 0x07,ZERO
	INT_VECTOR 0x08,ERROR_CODE
	INT_VECTOR 0x09,ZERO
	INT_VECTOR 0x0a,ERROR_CODE
	INT_VECTOR 0x0b,ERROR_CODE
	INT_VECTOR 0x0c,ZERO
	INT_VECTOR 0x0d,ERROR_CODE
	INT_VECTOR 0x0e,ERROR_CODE
	INT_VECTOR 0x0f,ZERO
	INT_VECTOR 0x10,ZERO
	INT_VECTOR 0x11,ERROR_CODE
	INT_VECTOR 0x12,ZERO
	INT_VECTOR 0x13,ZERO
	INT_VECTOR 0x14,ZERO
	INT_VECTOR 0x15,ZERO
	INT_VECTOR 0x16,ZERO
	INT_VECTOR 0x17,ZERO
	INT_VECTOR 0x18,ERROR_CODE
	INT_VECTOR 0x19,ZERO
	INT_VECTOR 0x1a,ERROR_CODE
	INT_VECTOR 0x1b,ERROR_CODE
	INT_VECTOR 0x1c,ZERO
	INT_VECTOR 0x1d,ERROR_CODE
	INT_VECTOR 0x1e,ERROR_CODE
	INT_VECTOR 0x1f,ZERO
	;主片发起的中断
	INT_VECTOR 0x20,ZERO ;时钟中断
	INT_VECTOR 0x21,ZERO ;键盘中断
	INT_VECTOR 0x22,ZERO ;级联
	INT_VECTOR 0x23,ZERO ;串口2
	INT_VECTOR 0x24,ZERO ;串口1
	INT_VECTOR 0x25,ZERO ;并口2
	INT_VECTOR 0x26,ZERO ;软盘中断
	INT_VECTOR 0x27,ZERO ;并口1
	;从片发起的中断
	INT_VECTOR 0x28,ZERO ;实时时钟 终端
	INT_VECTOR 0x29,ZERO ;重定向
	INT_VECTOR 0x2a,ZERO ;保留
	INT_VECTOR 0x2b,ZERO ;保留
	INT_VECTOR 0x2c,ZERO ;ps/2鼠标中断
	INT_VECTOR 0x2d,ZERO ;fpu浮点单元异常
	INT_VECTOR 0x2e,ZERO ;硬盘中断
	INT_VECTOR 0x2f,ZERO ;保留

section .text
global int_exit
int_exit:
	add esp,4
	popad
	pop gs
	pop fs
	pop es
	pop ds
	add esp,4
	iretd


extern syscall_table
global syscall_entry
syscall_entry:
	ZERO
	push ds
	push es
	push fs
	push gs
	pushad 				;按顺序压入 eax,ecx,edx,ebx,esp,ebp,esi,edi
	push 0x80
	push edx
	push ecx
	push ebx
	call [syscall_table+eax*4]
	add esp,0xc
	mov [esp+4*8],eax 	;修改栈中压入的eax的值
	jmp int_exit