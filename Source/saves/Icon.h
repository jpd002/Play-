#ifndef _ICON_H_
#define _ICON_H_

#include "Types.h"
#include <iostream>

class CIcon
{
public:
	struct VERTEX
	{
		double		nX;
		double		nY;
		double		nZ;
	};
	
	struct TEXCOORD
	{
		double		nS;
		double		nT;
	};

	struct KEY
	{
		float		nTime;
		float		nAmplitude;
	};

	struct FRAME
	{
		uint32		nShapeId;
		uint32		nKeyCount;
		KEY*		pKeys;
	};

					CIcon(const char*);
					~CIcon();

	const VERTEX*	GetShape(unsigned int)	const;
	const TEXCOORD*	GetTexCoords()			const;
	const FRAME*	GetFrame(unsigned int)	const;
	const uint16*	GetTexture()			const;
	unsigned int	GetShapeCount()			const;
	unsigned int	GetVertexCount()		const;
	unsigned int	GetFrameCount()			const;

private:
	void			ReadHeader(std::istream&);
	void			ReadVertices(std::istream&);
	void			ReadAnimations(std::istream&);
	void			ReadTexture(std::istream&);
	void			UncompressTexture(std::istream&);

	VERTEX**		m_pShapes;
	TEXCOORD*		m_pTexCoords;
	FRAME*			m_pFrames;

	uint32			m_nShapeCount;
	uint32			m_nVertexCount;
	uint32			m_nFrameCount;
	uint32			m_nTextureType;
	uint16*			m_pTexture;
};

#endif
