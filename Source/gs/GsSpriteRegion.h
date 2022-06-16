#pragma once

#include <cassert>
#include <vector>

class CGsSpriteRect
{
public:
	CGsSpriteRect(float x1 = 0, float y1 = 0, float x2 = 0, float y2 = 0)
	    : x1(x1)
	    , y1(y1)
	    , x2(x2)
	    , y2(y2)
	{
		assert(x1 <= x2);
		assert(y1 <= y2);
	}

	inline float GetWidth() const
	{
		return x2 - x1;
	}

	inline float GetHeight() const
	{
		return y2 - y1;
	}

	inline bool Intersects(const CGsSpriteRect& other) const
	{
		float rx1 = std::max<float>(x1, other.x1);
		float rx2 = std::min<float>(x2, other.x2);
		float ry1 = std::max<float>(y1, other.y1);
		float ry2 = std::min<float>(y2, other.y2);

		if(rx1 < rx2 && ry1 < ry2)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	float x1 = 0;
	float y1 = 0;
	float x2 = 0;
	float y2 = 0;
};

class CGsSpriteRegion
{
public:
	void Reset()
	{
		m_spriteRects.clear();
	}

	inline bool Intersects(const CGsSpriteRect& rect) const
	{
		for(const auto& spriteRect : m_spriteRects)
		{
			if(spriteRect.Intersects(rect))
			{
				return true;
			}
		}
		return false;
	}

	void Insert(const CGsSpriteRect& rect)
	{
		for(auto& spriteRect : m_spriteRects)
		{
			if(
			    (spriteRect.x1 == rect.x1) &&
			    (spriteRect.x2 == rect.x2))
			{
				//rect & spriteRect share the same horizontal edges
				spriteRect.y1 = std::min<float>(spriteRect.y1, rect.y1);
				spriteRect.y2 = std::max<float>(spriteRect.y2, rect.y2);
				return;
			}
			if(
			    (spriteRect.y1 == rect.y1) &&
			    (spriteRect.y2 == rect.y2))
			{
				//rect & spriteRect share the same vertical edges
				spriteRect.x1 = std::min<float>(spriteRect.x1, rect.x1);
				spriteRect.x2 = std::max<float>(spriteRect.x2, rect.x2);
				return;
			}
		}
		m_spriteRects.push_back(rect);
	}

private:
	std::vector<CGsSpriteRect> m_spriteRects;
};
