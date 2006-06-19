PUBLIC _cpuid_GetCpuFeatures
PUBLIC _cpuid_GetCpuIdString

.CODE

_cpuid_GetCpuFeatures	PROC
	mov		eax, 001h
	cpuid
	mov		eax, edx
	ret
_cpuid_GetCpuFeatures	ENDP

_cpuid_GetCpuIdString	PROC
	push	rbx

	xor		eax, eax
	mov		r9, rcx
	cpuid
	mov		dword ptr[r9 + 000h], ebx
	mov		dword ptr[r9 + 004h], edx
	mov		dword ptr[r9 + 008h], ecx
	
	pop		rbx

	ret
_cpuid_GetCpuIdString	ENDP

END
