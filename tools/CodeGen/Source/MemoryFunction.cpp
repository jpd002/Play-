#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "MemoryFunction.h"

#ifdef WIN32

#include <windows.h>

#ifdef AMD64
extern "C" void _CMemoryFunction_Execute(void*, void*);
#endif

#elif defined(__APPLE__)

#include <mach/mach_init.h>
#include <mach/vm_map.h>
extern "C" void __clear_cache(void* begin, void* end);

#endif

CMemoryFunction::CMemoryFunction(const void* code, size_t size)
: m_code(NULL)
{
#ifdef WIN32
	m_size = size;
	m_code = malloc(size);
	memcpy(m_code, code, size);
	
	DWORD oldProtect = 0;
	BOOL result = VirtualProtect(m_code, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	assert(result == TRUE);
#elif defined(__APPLE__)
	vm_size_t page_size = 0;
	host_page_size(mach_task_self(), &page_size);
	unsigned int allocSize = ((size + page_size - 1) / page_size) * page_size;
	vm_allocate(mach_task_self(), reinterpret_cast<vm_address_t*>(&m_code), allocSize, TRUE); 
	memcpy(m_code, code, size);
	__clear_cache(m_code, reinterpret_cast<uint8*>(m_code) + size);
	kern_return_t result = vm_protect(mach_task_self(), reinterpret_cast<vm_address_t>(m_code), size, 0, VM_PROT_READ | VM_PROT_EXECUTE);
	assert(result == 0);
	m_size = allocSize;
#endif
}

CMemoryFunction::~CMemoryFunction()
{
    if(m_code != NULL)
    {
#ifdef WIN32
		free(m_code);
#elif defined(__APPLE__)
		vm_deallocate(mach_task_self(), reinterpret_cast<vm_address_t>(m_code), m_size); 
#endif
    }
}

void CMemoryFunction::operator()(void* context)
{

#ifdef WIN32
	
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
	
#elif defined(__APPLE__)
	
	#if !(TARGET_IPHONE_SIMULATOR)
	
		volatile const void* code = m_code;
		volatile const void* dataPtr = context;
	
		__asm__ ("mov r1, %0" : : "r"(dataPtr) : "r1");
		__asm__ ("mov r0, %0" : : "r"(code) : "r0");
		__asm__ ("stmdb sp!, {r2, r3, r4, r5, r6, r7, r11, ip}");
		__asm__ ("mov r11, r1");
		__asm__ ("blx r0");
		__asm__ ("ldmia sp!, {r2, r3, r4, r5, r6, r7, r11, ip}");
	
	#else
	
	#endif
	
#endif

}
