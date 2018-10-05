#pragma once

#include "Types.h"
#include "Stream.h"
#include <boost/filesystem.hpp>

class CMdsDiscImage
{
public:
	CMdsDiscImage(const boost::filesystem::path&);

private:
	bool m_isDualLayer = false;
	uint32 m_layerBreak = 0;

	void ParseMds(Framework::CStream&);
};
