[bits 32]
section .text
;void switch_to(task_struct* cur_pcb,task_struct* new_pcb);
global switch_to
switch_to:
	push esi
	push edi
	push ebx
	push ebp

	mov eax,[esp+20]
	mov [eax],esp			;save old_esp to cur_pcb->self_kernel_stack
	mov eax,[esp+24]
	mov esp,[eax]			;mov new_pcb->self_kernel_stack to esp

	pop ebp
	pop ebx	
	pop edi
	pop esi
	ret
extern freePcb
global switch_to_and_free
global switch_to_and_free_end
switch_to_and_free:
        push esi
        push edi
        push ebx
        push ebp

        mov eax,[esp+20]
        mov ebx,[esp+24]
	mov [eax],esp                   ;save old_esp to cur_pcb->self_kernel_stack
        push esp
	call freePcb
switch_to_and_free_end:
	mov esp,[ebx]                   ;mov new_pcb->self_kernel_stack to esp

        pop ebp
        pop ebx
        pop edi
        pop esi
        ret

