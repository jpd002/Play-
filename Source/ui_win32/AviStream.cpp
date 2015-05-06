#include "AviStream.h"

CAviStream::CAviStream(unsigned int nWidth, unsigned int nHeight)
: m_pFile(NULL)
, m_pStream(NULL)
, m_pCompStream(NULL)
, m_nOpen(false)
{
	AVIFileInit();
	SetSize(nWidth, nHeight);
}

CAviStream::~CAviStream()
{
	Close();
	AVIFileExit();
}

void CAviStream::SetSize(unsigned int nWidth, unsigned int nHeight)
{
	m_nWidth = nWidth;
	m_nHeight = nHeight;

	memset(&m_Format, 0, sizeof(BITMAPINFOHEADER));
	m_Format.biSize			= sizeof(BITMAPINFOHEADER);
	m_Format.biWidth		= nWidth;
	m_Format.biHeight		= nHeight;
	m_Format.biPlanes		= 1;
	m_Format.biBitCount		= 24;
	m_Format.biSizeImage	= nWidth * nHeight * 4;
	m_Format.biCompression	= BI_RGB;
}

bool CAviStream::Open(HWND parentWnd, const TCHAR* sFilename)
{
	assert(m_nOpen == false);
	if(m_nOpen) return false;

	m_nOpen = true;

	if(AVIFileOpen(&m_pFile, sFilename, OF_CREATE | OF_WRITE, NULL))
	{
		Close();
		return false;
	}

	AVISTREAMINFO si;
	memset(&si, 0, sizeof(AVISTREAMINFO));
	si.fccType					= streamtypeVIDEO;
	si.fccHandler				= 0;
	si.dwScale					= 1;
	si.dwRate					= 60;			//60 frames per second
	si.dwSuggestedBufferSize	= m_nWidth * m_nHeight * 4;
	SetRect(&si.rcFrame, 0, 0, m_nWidth, m_nHeight);

	if(AVIFileCreateStream(m_pFile, &m_pStream, &si))
	{
		Close();
		return false;
	}

	AVICOMPRESSOPTIONS co;
	memset(&co, 0, sizeof(AVICOMPRESSOPTIONS));
	AVICOMPRESSOPTIONS* pco = &co;
	if(!AVISaveOptions(parentWnd, 0, 1, &m_pStream, &pco))
	{
		Close();
		return false;
	}

	if(AVIMakeCompressedStream(&m_pCompStream, m_pStream, &co, NULL))
	{
		Close();
		return false;
	}
	AVISaveOptionsFree(1, &pco);

	if(AVIStreamSetFormat(m_pCompStream, 0, &m_Format, m_Format.biSize))
	{
		Close();
		return false;
	}

	m_nFrame = 0;
	return true;
}

void CAviStream::Write(unsigned char* pBuffer)
{
	AVIStreamWrite(m_pCompStream, m_nFrame, 1, pBuffer, m_Format.biSizeImage, AVIIF_KEYFRAME, NULL, NULL);
	m_nFrame++;
}

void CAviStream::Close()
{
	if(m_nOpen)
	{
		if(m_pCompStream != NULL)
		{
			AVIStreamRelease(m_pCompStream);
			m_pCompStream = NULL;
		}
		if(m_pStream != NULL)
		{
			AVIStreamRelease(m_pStream);
			m_pStream = NULL;
		}
		if(m_pFile != NULL)
		{
			AVIFileRelease(m_pFile);
			m_pFile = NULL;
		}
		m_nOpen = false;
	}
}
