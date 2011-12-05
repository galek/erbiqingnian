.CODE             ;Indicates the start of a code segment.

ALIGN 8
VMCallCDecl PROC \
	;rcx=vm:QWORD,			; VMSTATE*	
	;rdx=args:QWORD,		; const void*
	;r8=sz:QWORD,			; size_t 
	;r9=func:QWORD			; void*

	; across function calls RBX, RBP, RDI, RSI, R12, R13, R14, and R15 must be
	; preserved. Out of these registers, we only use RDI, RSI, and R12, R15.
	push rdi
	push rsi
	push r12
	push r15
		
	sub rsp, r8				; allocate stack space
	sub rsp, 20h
	
	; the stack must be 16-byte aligned
	mov rax, rsp
	and rax, 0Fh
	sub rsp, rax
	
	; preserve rsp
	mov r12, rax			; add stack adjustment for 16-byte alignment
	add r12, r8				; add size of args buffer
	
	mov rsi, rdx			; start of source stack frame
	mov rdi, rsp			; start of dest stack frame
	mov rcx, r8				; r8 has the size of the args buffer
	shr rcx, 3				; size of args in qwords	

	rep movsq				; copy params to stack	
	
	mov r15, r9				
	
	mov rcx, QWORD PTR [rsp]	; copy first four params to registers
	mov rdx, QWORD PTR [rsp+8]
	mov r8, QWORD PTR [rsp+16]
	mov r9, QWORD PTR [rsp+24]

	call r15				; call function - leave DWORD retval in RAX	
	
	add rsp, r12			; restore stack pointer
	add rsp, 20h
	
	; restore preserved registers for the caller
	pop r15
	pop r12
	pop rsi
	pop rdi
	
	ret
VMCallCDecl ENDP

;ALIGN 8
;VMCallStdCall PROC \
;	vm:DWORD,			; VMSTATE*
;	args:DWORD,			; const void*
;	sz:DWORD,			; size_t
;	func:DWORD			; void*
	
;	mov ecx, sz			; size of args buffer
;	mov esi, args		; buffer
;	sub esp, ecx		; allocate stack space
;	mov edi, esp		; start of dest stack frame
;	shr ecx, 2			; size of args in dwords
;	rep movsd			; copy params to stack
;	call [func]			; call function - leave DWORD retval in EAX
	
;	ret	
;VMCallStdCall ENDP

;ALIGN 8
;GetST0 PROC
;	LOCAL f:DWORD

;	fstp dword ptr [f]			; pop STO into f
;	mov eax, dword ptr [f]		; copy int eax
	
;	ret							; done
;GetST0 ENDP

END 