#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/stream.hpp>
#include <exception>
#include "Icon.h"

using namespace boost;
using namespace std;

CIcon::CIcon(const char* sPath)
{
	iostreams::stream_buffer<iostreams::file_source> IconFileBuffer(sPath, ios::in | ios::binary);
	istream IconFile(&IconFileBuffer);

	ReadHeader(IconFile);
	ReadVertices(IconFile);
	ReadAnimations(IconFile);
	ReadTexture(IconFile);
}

CIcon::~CIcon()
{
	delete [] m_pTexture;
	delete [] m_pTexCoords;

	for(unsigned int i = 0; i < m_nFrameCount; i++)
	{
		delete [] m_pFrames[i].pKeys;
	}

	delete [] m_pFrames;

	for(unsigned int i = 0; i < m_nShapeCount; i++)
	{
		delete [] m_pShapes[i];
	}

	delete [] m_pShapes;
}

const CIcon::VERTEX* CIcon::GetShape(unsigned int nShapeIdx) const
{
	return m_pShapes[nShapeIdx];
}

const CIcon::TEXCOORD* CIcon::GetTexCoords() const
{
	return m_pTexCoords;
}

const CIcon::FRAME* CIcon::GetFrame(unsigned int nFrameIdx) const
{
	return &m_pFrames[nFrameIdx];
}

const uint16* CIcon::GetTexture() const
{
	return m_pTexture;
}

unsigned int CIcon::GetShapeCount() const
{
	return m_nShapeCount;
}

unsigned int CIcon::GetVertexCount() const
{
	return m_nVertexCount;
}

unsigned int CIcon::GetFrameCount() const
{
	return m_nFrameCount;
}

void CIcon::ReadHeader(istream& Input)
{
	uint32 nMagic;

	Input.read(reinterpret_cast<char*>(&nMagic), 4);
	if(nMagic != 0x010000)
	{
		throw exception("Invalid icon file");
	}

	Input.read(reinterpret_cast<char*>(&m_nShapeCount), 4);
	Input.read(reinterpret_cast<char*>(&m_nTextureType), 4);
	Input.seekg(4, ios::cur);
	Input.read(reinterpret_cast<char*>(&m_nVertexCount), 4);

	if((m_nTextureType != 0x06) &&
		(m_nTextureType != 0x07) &&
		(m_nTextureType != 0x0C) &&
		(m_nTextureType != 0x0E) &&
		(m_nTextureType != 0x0F))
	{
		throw exception("Unknown texture format.");
	}
}

void CIcon::ReadVertices(istream& Input)
{
	m_pShapes = new VERTEX*[m_nShapeCount];
	for(unsigned int j = 0; j < m_nShapeCount; j++)
	{
		m_pShapes[j] = new VERTEX[m_nVertexCount];
	}

	m_pTexCoords = new TEXCOORD[m_nVertexCount];

	for(unsigned int i = 0; i < m_nVertexCount; i++)
	{
		for(unsigned int j = 0; j < m_nShapeCount; j++)
		{
			int16 nX, nY, nZ;
			VERTEX* pVertex;

			pVertex = &m_pShapes[j][i];
			Input.read(reinterpret_cast<char*>(&nX), 2);
			Input.read(reinterpret_cast<char*>(&nY), 2);
			Input.read(reinterpret_cast<char*>(&nZ), 2);
			Input.seekg(2, ios::cur);

			pVertex->nX = (double)nX / 4096.0;
			pVertex->nY = (double)nY / 4096.0;
			pVertex->nZ = (double)nZ / 4096.0;
		}

		//Skip normals
		Input.seekg(8, ios::cur);

		//Read texture coordinates
		int16 nS, nT;
		Input.read(reinterpret_cast<char*>(&nS), 2);
		Input.read(reinterpret_cast<char*>(&nT), 2);

		m_pTexCoords[i].nS = (double)nS / 4096.0;
		m_pTexCoords[i].nT = (double)nT / 4096.0;

		//Skip color
		Input.seekg(4, ios::cur);
	}
}

void CIcon::ReadAnimations(istream& Input)
{
	uint32 nMagic;

	Input.read(reinterpret_cast<char*>(&nMagic), 4);
	
	if(nMagic != 0x01)
	{
		throw exception("Invalid icon file (Animation Header Broken).");
	}

	Input.seekg(12, ios::cur);
	Input.read(reinterpret_cast<char*>(&m_nFrameCount), 4);

	m_pFrames = new FRAME[m_nFrameCount];

	for(unsigned int i = 0; i < m_nFrameCount; i++)
	{
		FRAME* pFrame;

		pFrame = &m_pFrames[i];

		Input.read(reinterpret_cast<char*>(&pFrame->nShapeId), 4);
		Input.read(reinterpret_cast<char*>(&pFrame->nKeyCount), 4);

		pFrame->pKeys = new KEY[pFrame->nKeyCount];

		for(unsigned int j = 0; j < pFrame->nKeyCount; j++)
		{
			KEY* pKey;

			pKey = &pFrame->pKeys[j];

			Input.read(reinterpret_cast<char*>(&pKey->nTime),		4);
			Input.read(reinterpret_cast<char*>(&pKey->nAmplitude),	4);
		}
	}
}

void CIcon::ReadTexture(istream& Input)
{
	m_pTexture = new uint16[128 * 128];

	switch(m_nTextureType)
	{
	case 6:
	case 7:
		Input.read(reinterpret_cast<char*>(m_pTexture), 128 * 128 * 2);
		break;
	case 12:
	case 14:
	case 15:
		UncompressTexture(Input);
		break;
	}
}

void CIcon::UncompressTexture(istream& Input)
{
	uint32 nDataLength;
	int nEndPosition;
	unsigned int nTexPosition;

	Input.read(reinterpret_cast<char*>(&nDataLength), 4);

	nEndPosition = (int)Input.tellg() + nDataLength;
	nTexPosition = 0;

	while((Input.tellg() < nEndPosition) && (!Input.eof()))
	{
		uint16 nCode;

		Input.read(reinterpret_cast<char*>(&nCode), 2);

		if((nCode & 0xFF00) == 0xFF00)
		{
			uint16 nLength;
			nLength = -nCode;
			for(unsigned int i = 0; i < nLength; i++)
			{
				Input.read(reinterpret_cast<char*>(&m_pTexture[nTexPosition++]), 2);
			}
		}
		else
		{
			uint16 nValue;
			
			Input.read(reinterpret_cast<char*>(&nValue), 2);
			for(unsigned int i = 0; i < nCode; i++)
			{
				m_pTexture[nTexPosition++] = nValue;
			}
		}
	}
}
