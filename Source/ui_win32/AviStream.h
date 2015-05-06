#ifndef _AVISTREAM_H_
#define _AVISTREAM_H_

#include <windows.h>
#include <vfw.h>

class CAviStream
{
public:
						CAviStream(unsigned int = 0, unsigned int = 0);
	virtual				~CAviStream();

	bool				Open(HWND, const TCHAR*);
	void				Write(unsigned char*);
	void				SetSize(unsigned int, unsigned int);
	void				Close();

private:

	IAVIFile*			m_pFile;
	IAVIStream*			m_pStream;
	IAVIStream*			m_pCompStream;
	BITMAPINFOHEADER	m_Format;
	bool				m_nOpen;
	unsigned int		m_nFrame;
	unsigned int		m_nWidth;
	unsigned int		m_nHeight;
};

#endif
