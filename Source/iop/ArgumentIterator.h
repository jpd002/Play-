#pragma once

#include "../MIPS.h"

class CArgumentIterator
{
public:
	virtual ~CArgumentIterator() = default;
	virtual uint32 GetNext() = 0;
};

class CCallArgumentIterator : public CArgumentIterator
{
public:
	CCallArgumentIterator(CMIPS&);

	uint32 GetNext() override;

private:
	CMIPS& m_context;
	unsigned int m_current = 0;
};

class CValistArgumentIterator : public CArgumentIterator
{
public:
	CValistArgumentIterator(CMIPS&, uint32);

	uint32 GetNext() override;

private:
	CMIPS& m_context;
	uint32 m_argsPtr = 0;
};
