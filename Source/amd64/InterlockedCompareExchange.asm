; The current version of Boost (1.33.1) requires this to compile successfully
;

PUBLIC _InterlockedCompareExchange

.CODE

_InterlockedCompareExchange	PROC
	mov				eax, r8d
	lock cmpxchg	dword ptr[rcx], edx
	ret
_InterlockedCompareExchange	ENDP

END
