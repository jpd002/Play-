#pragma once

#include "Types.h"
#include <vector>
#include <functional>

#define TEST_VERIFY(a)                                        \
	if(!(a))                                                  \
	{                                                         \
		printf("Verification failed: '%s'. Aborting.\n", #a); \
		std::abort();                                         \
	}

class CQtMemoryViewModelTests
{
public:
	void RunTests();

private:
	uint8 GetByte(uint32);

	void TestSimpleBytes();
	void TestOutOfBounds();
	void TestAddressToModelIndex();
	void TestAddressToModelIndexWord();
	void TestModelIndexToAddress();
	void TestModelIndexToAddressWord();

	std::function<uint8(uint32)> m_getByteFunction;
	std::vector<uint8> m_memory;
};
