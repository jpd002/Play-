#pragma once

#include <Windows.h>
#include "../MipsExecutor.h"

class CEeExecutor : public CMipsExecutor
{
public:
							CEeExecutor(CMIPS&, uint8*);
	virtual					~CEeExecutor();

	void					Reset() override;
	void					ClearActiveBlocksInRange(uint32, uint32) override;

	BasicBlockPtr			BlockFactory(CMIPS&, uint32, uint32) override;

	static LONG CALLBACK	HandleException(_EXCEPTION_POINTERS*);

private:
	LONG					HandleExceptionInternal(_EXCEPTION_POINTERS*);

	uint8*					m_ram = nullptr;
	uint32					m_pageSize = 0;
	LPVOID					m_handler = NULL;
};
