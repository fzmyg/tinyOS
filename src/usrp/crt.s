[bits 32]
section .text
global _start
extern main
extern exit
_start:
    push ebx
    push ecx
    call main
    push eax
    call exit