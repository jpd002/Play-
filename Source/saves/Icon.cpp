#include <exception>
#include <cassert>
#include "Icon.h"

#pragma pack(push, 1)

struct FILEVERTEX
{
	int16 nX;
	int16 nY;
	int16 nZ;
	int16 nW;
};

struct FILEVERTEXATTRIB
{
	int16 nNX;
	int16 nNY;
	int16 nNZ;
	int16 nNW;
	int16 nS;
	int16 nT;
	uint32 nColor;
};

#pragma pack(pop)

CIcon::CIcon(Framework::CStream& stream)
    : m_pTexture(NULL)
    , m_pTexCoords(NULL)
    , m_pFrames(NULL)
    , m_pShapes(NULL)
    , m_nShapeCount(0)
    , m_nVertexCount(0)
    , m_nTextureType(0)
    , m_nFrameCount(0)
{
	ReadHeader(stream);
	ReadVertices(stream);
	ReadAnimations(stream);
	ReadTexture(stream);
}

CIcon::~CIcon()
{
	delete[] m_pTexture;
	delete[] m_pTexCoords;

	for(unsigned int i = 0; i < m_nFrameCount; i++)
	{
		delete[] m_pFrames[i].pKeys;
	}

	delete[] m_pFrames;

	for(unsigned int i = 0; i < m_nShapeCount; i++)
	{
		delete[] m_pShapes[i];
	}

	delete[] m_pShapes;
}

const CIcon::VERTEX* CIcon::GetShape(unsigned int nShapeIdx) const
{
	assert(nShapeIdx < m_nShapeCount);
	if(nShapeIdx >= m_nShapeCount) return nullptr;
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

void CIcon::ReadHeader(Framework::CStream& iconStream)
{
	uint32 nMagic = iconStream.Read32();
	if(nMagic != 0x010000)
	{
		throw std::runtime_error("Invalid icon file");
	}

	m_nShapeCount = iconStream.Read32();
	m_nTextureType = iconStream.Read32();
	iconStream.Seek(4, Framework::STREAM_SEEK_CUR);
	m_nVertexCount = iconStream.Read32();

	if((m_nTextureType != 0x06) &&
	   (m_nTextureType != 0x07) &&
	   (m_nTextureType != 0x0C) &&
	   (m_nTextureType != 0x0E) &&
	   (m_nTextureType != 0x0F))
	{
		throw std::runtime_error("Unknown texture format.");
	}
}

void CIcon::ReadVertices(Framework::CStream& iconStream)
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
			VERTEX* pVertex = &m_pShapes[j][i];

			FILEVERTEX fileVertex;
			iconStream.Read(&fileVertex, sizeof(FILEVERTEX));

			pVertex->nX = static_cast<float>(fileVertex.nX) / 4096.0f;
			pVertex->nY = static_cast<float>(fileVertex.nY) / 4096.0f;
			pVertex->nZ = static_cast<float>(fileVertex.nZ) / 4096.0f;
		}

		FILEVERTEXATTRIB fileVertexAttrib;
		iconStream.Read(&fileVertexAttrib, sizeof(FILEVERTEXATTRIB));

		m_pTexCoords[i].nS = static_cast<float>(fileVertexAttrib.nS) / 4096.0f;
		m_pTexCoords[i].nT = static_cast<float>(fileVertexAttrib.nT) / 4096.0f;
	}
}

void CIcon::ReadAnimations(Framework::CStream& iconStream)
{
	uint32 nMagic = iconStream.Read32();

	if(nMagic != 0x01)
	{
		throw std::runtime_error("Invalid icon file (Animation Header Broken).");
	}

	iconStream.Seek(12, Framework::STREAM_SEEK_CUR);
	m_nFrameCount = iconStream.Read32();

	m_pFrames = new FRAME[m_nFrameCount];

	for(unsigned int i = 0; i < m_nFrameCount; i++)
	{
		FRAME* pFrame = &m_pFrames[i];

		pFrame->nShapeId = iconStream.Read32();
		pFrame->nKeyCount = iconStream.Read32();

		pFrame->pKeys = new KEY[pFrame->nKeyCount];
		iconStream.Read(pFrame->pKeys, sizeof(KEY) * pFrame->nKeyCount);
	}
}

void CIcon::ReadTexture(Framework::CStream& iconStream)
{
	m_pTexture = new uint16[128 * 128];

	switch(m_nTextureType)
	{
	case 6:
	case 7:
		iconStream.Read(m_pTexture, 128 * 128 * 2);
		break;
	case 12:
	case 14:
	case 15:
		UncompressTexture(iconStream);
		break;
	}
}

void CIcon::UncompressTexture(Framework::CStream& iconStream)
{
	uint32 nDataLength = iconStream.Read32();

	int nEndPosition = static_cast<int>(iconStream.Tell()) + nDataLength;
	unsigned int nTexPosition = 0;

	while((iconStream.Tell() < nEndPosition) && !iconStream.IsEOF())
	{
		uint16 nCode = iconStream.Read16();

		if((nCode & 0xFF00) == 0xFF00)
		{
			uint16 nLength = -nCode;
			for(unsigned int i = 0; i < nLength; i++)
			{
				iconStream.Read(&m_pTexture[nTexPosition++], 2);
			}
		}
		else
		{
			uint16 nValue = iconStream.Read16();
			for(unsigned int i = 0; i < nCode; i++)
			{
				m_pTexture[nTexPosition++] = nValue;
			}
		}
	}
}
