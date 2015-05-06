#pragma once

#include "../RegViewPage.h"
#include "Types.h"

class CGifPacketView : public CRegViewPage
{
public:
									CGifPacketView(HWND, const RECT&);
	virtual							~CGifPacketView();

	void							SetPacket(uint8*, uint32);
};
