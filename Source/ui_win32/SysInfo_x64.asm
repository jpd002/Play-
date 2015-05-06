PUBLIC _SysInfo_CPUID

.CODE

_SysInfo_CPUID	PROC
	push r12
	mov r12, rdx
	mov rax, rcx
	cpuid
	mov dword ptr[r12 + 0], ebx;
	mov dword ptr[r12 + 4], edx;
	mov dword ptr[r12 + 8], ecx;
	pop r12
	ret
_SysInfo_CPUID	ENDP

END
