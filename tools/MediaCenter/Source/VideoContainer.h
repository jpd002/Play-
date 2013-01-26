#pragma once

class CVideoContainer
{
public:
	enum STATUS
	{
		STATUS_INTERRUPTED,
		STATUS_ERROR,
		STATUS_EOF
	};

	virtual						~CVideoContainer() {}
	virtual STATUS				Read() = 0;
};
