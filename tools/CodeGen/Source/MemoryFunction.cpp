#ifdef WIN32
#include <windows.h>
#endif

#ifdef AMD64
extern "C" void _CMemoryFunction_Execute(void*, void*);
#endif

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "MemoryFunction.h"

CMemoryFunction::CMemoryFunction(const void* code, size_t size)
{
	m_code = malloc(size);
	memcpy(m_code, code, size);
#ifdef WIN32
	DWORD oldProtect = 0;
	BOOL result = VirtualProtect(m_code, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	assert(result == TRUE);
#else
	
#endif
}

CMemoryFunction::~CMemoryFunction()
{
	free(m_code);
}

void CMemoryFunction::operator()(void* context)
{
#ifdef AMD64

	_CMemoryFunction_Execute(m_code, context);

#else

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

#endif

}
