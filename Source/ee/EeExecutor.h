#pragma once

#ifdef _MSC_VER
#include <Windows.h>
#endif

#include "../MipsExecutor.h"

class CEeExecutor : public CMipsExecutor
{
public:
							CEeExecutor(CMIPS&, uint8*);
	virtual					~CEeExecutor();

	void					Reset() override;
	void					ClearActiveBlocksInRange(uint32, uint32) override;

	BasicBlockPtr			BlockFactory(CMIPS&, uint32, uint32) override;

private:
	uint8*					m_ram = nullptr;
	uint32					m_pageSize = 0;

	void					SetMemoryProtected(void*, size_t, bool);

#ifdef _MSC_VER
	static LONG CALLBACK	HandleException(_EXCEPTION_POINTERS*);
	LONG					HandleExceptionInternal(_EXCEPTION_POINTERS*);

	LPVOID					m_handler = NULL;
#else
	static void				HandleException(int, siginfo_t*, void*);
	void					HandleExceptionInternal(int, siginfo_t*, void*);
#endif
};
