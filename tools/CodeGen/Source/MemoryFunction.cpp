#include <stdlib.h>
#include <string.h>
#include "MemoryFunction.h"

CMemoryFunction::CMemoryFunction(const void* code, size_t size)
{
	m_code = malloc(size);
	memcpy(m_code, code, size);
}

CMemoryFunction::~CMemoryFunction()
{
	free(m_code);
}

void CMemoryFunction::operator()(void* context)
{
	volatile const void* code = m_code;
	volatile const void* dataPtr = context;

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
}
