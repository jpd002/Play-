#pragma once

#include "Types.h"
#include "Stream.h"
#include <memory>

class CIcon
{
public:
	struct VERTEX
	{
		float nX;
		float nY;
		float nZ;
	};

	struct TEXCOORD
	{
		float nS;
		float nT;
	};

#pragma pack(push, 1)

	struct KEY
	{
		float nTime;
		float nAmplitude;
	};

#pragma pack(pop)

	struct FRAME
	{
		uint32 nShapeId;
		uint32 nKeyCount;
		KEY* pKeys;
	};

	CIcon(Framework::CStream&);
	virtual ~CIcon();

	const VERTEX* GetShape(unsigned int) const;
	const TEXCOORD* GetTexCoords() const;
	const FRAME* GetFrame(unsigned int) const;
	const uint16* GetTexture() const;
	unsigned int GetShapeCount() const;
	unsigned int GetVertexCount() const;
	unsigned int GetFrameCount() const;

private:
	void ReadHeader(Framework::CStream&);
	void ReadVertices(Framework::CStream&);
	void ReadAnimations(Framework::CStream&);
	void ReadTexture(Framework::CStream&);
	void UncompressTexture(Framework::CStream&);

	VERTEX** m_pShapes = nullptr;
	TEXCOORD* m_pTexCoords = nullptr;
	FRAME* m_pFrames = nullptr;

	uint32 m_nShapeCount = 0;
	uint32 m_nVertexCount = 0;
	uint32 m_nFrameCount = 0;
	uint32 m_nTextureType = 0;
	uint16* m_pTexture = nullptr;
};

typedef std::shared_ptr<CIcon> IconPtr;
