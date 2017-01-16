#pragma once

class CGsCachedAreaTest
{
public:
	void Execute();

private:
	void CheckEmptyArea();
	void CheckDirtyRect();
	void CheckClearDirtyPages();
	void CheckInvalidate();
};
