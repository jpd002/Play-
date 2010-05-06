#ifdef AMD64
#include <windows.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "MemoryFunction.h"

CMemoryFunction::CMemoryFunction(const void* code, size_t size)
{
#ifdef AMD64
	m_code = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	memcpy(m_code, code, size);
#else
	m_code = malloc(size);
	memcpy(m_code, code, size);
#endif
}

CMemoryFunction::~CMemoryFunction()
{
#ifdef AMD64
	assert(0);
#else
	free(m_code);
#endif
}

void CMemoryFunction::operator()(void* context)
{
	volatile const void* code = m_code;
	volatile const void* dataPtr = context;

#ifdef AMD64

	assert(0);

#else

	__asm
	{
		push ebp
		push ebx
		push esi
		push edi

		mov eax, code
		mov ebp, dataPtr
		call eax

		pop edi
		pop esi
		pop ebx
		pop ebp
	}

#endif

}
