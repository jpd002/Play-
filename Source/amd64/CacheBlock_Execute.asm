PUBLIC _CCacheBlock_Execute

.CODE

_CCacheBlock_Execute	PROC
	push	rbx
	push	rsi
	push	rdi
	push	rbp
	
	mov		rbp, rdx
	call	rcx
	
	pop		rbp
	pop		rbx
	pop		rsi
	pop		rdi

	ret
_CCacheBlock_Execute	ENDP

END
