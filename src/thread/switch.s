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

