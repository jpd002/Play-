#ifndef _GIF_H_
#define _GIF_H_

#include "Types.h"

class CGIF
{
public:
	static void				Reset();
	static uint32			ReceiveDMA(uint32, uint32, bool);
	static uint32			ProcessPacket(uint8*, uint32, uint32);

private:
	static uint32			ProcessPacked(uint8*, uint32, uint32);
	static uint32			ProcessRegList(uint8*, uint32, uint32);
	static uint32			ProcessImage(uint8*, uint32, uint32);

	static uint16			m_nLoops;
	static uint8			m_nCmd;
	static uint8			m_nRegs;
	static uint8			m_nRegsTemp;
	static uint64			m_nRegList;
	static bool				m_nEOP;
	static uint32			m_nQTemp;
};

#endif
