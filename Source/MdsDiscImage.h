#pragma once

#include "Types.h"
#include "Stream.h"

class CMdsDiscImage
{
public:
	CMdsDiscImage(Framework::CStream&);

	bool IsDualLayer() const;
	uint32 GetLayerBreak() const;

private:
	bool m_isDualLayer = false;
	uint32 m_layerBreak = 0;

	void ParseMds(Framework::CStream&);
};
