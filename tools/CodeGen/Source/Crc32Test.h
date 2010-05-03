#ifndef _CRC32TEST_H_
#define _CRC32TEST_H_

#include "Jitter.h"
#include "MemoryFunction.h"
#include <string>

class CCrc32Test
{
public:
						CCrc32Test(const char*, uint32);
	virtual				~CCrc32Test();

	void				Run();
	void				Compile(Jitter::CJitter&);

private:
	enum STATE
	{
		STATE_TEST,
		STATE_COMPUTE,
		STATE_DONE,
	};

	struct CONTEXT
	{
		uint32		nextByte;
		uint32		currentCrc;
		uint32		state;
	};

	void				CompileTestFunction(Jitter::CJitter&);
	void				CompileComputeFunction(Jitter::CJitter&);

	static uint32		GetNextByte(CCrc32Test*);
	static uint32		GetTableValue(CCrc32Test*, uint32);

	CONTEXT				m_context;
	CMemoryFunction*	m_testFunction;
	CMemoryFunction*	m_computeFunction;

	static void			BuildTable();

	static bool			m_tableBuilt;
	static uint32		m_table[0x100];

	std::string			m_input;
	unsigned int		m_inputPtr;
	uint32				m_result;
};

#endif
