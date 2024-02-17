#pragma once

class CScreenPositionListener
{
public:
	virtual ~CScreenPositionListener() = default;
	virtual void SetScreenPosition(float, float) = 0;
	virtual void ReleaseScreenPosition() = 0;
};
