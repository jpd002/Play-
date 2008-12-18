#ifndef _SH_WAVEOUT_H_
#define _SH_WAVEOUT_H_

#include <windows.h>
#include <mmsystem.h>
#include "SpuHandler.h"

class CSH_WaveOut : public CSpuHandler
{
public:
							CSH_WaveOut();
	virtual					~CSH_WaveOut();

	void					Reset();
	void					Update(Iop::CSpuBase&, Iop::CSpuBase&);
	bool					HasFreeBuffers();

private:
	enum
	{
		MAX_BUFFERS = 25,
	};

	WAVEHDR*				GetFreeBuffer();
	void					WaveOutProc(HWAVEOUT, UINT, DWORD_PTR, DWORD_PTR);
	static void	CALLBACK 	WaveOutProcStub(HWAVEOUT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR);

	HWAVEOUT				m_waveOut;
	WAVEHDR					m_buffer[MAX_BUFFERS];
	int16*					m_bufferMemory;
};

#endif
