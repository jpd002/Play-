#pragma once

class CAnalogueListener
{
public:
	virtual ~CAnalogueListener() = default;
	virtual void SetAnaloguePosition(float, float) = 0;
};
