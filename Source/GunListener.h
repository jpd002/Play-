#pragma once

class CGunListener
{
public:
	virtual ~CGunListener() = default;
	virtual void SetGunPosition(float, float) = 0;
};
