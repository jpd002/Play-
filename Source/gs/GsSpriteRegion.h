#pragma once

#include <cassert>
#include <vector>
#include <algorithm>

class CGsSpriteRect
{
public:
	CGsSpriteRect(float x1 = 0, float y1 = 0, float x2 = 0, float y2 = 0)
	    : left(std::min<float>(x1, x2))
	    , right(std::max<float>(x1, x2))
	    , top(std::min<float>(y1, y2))
	    , bottom(std::max<float>(y1, y2))
	{
		assert(left <= right);
		assert(top <= bottom);
	}

	inline float GetWidth() const
	{
		return right - left;
	}

	inline float GetHeight() const
	{
		return bottom - top;
	}

	inline bool Intersects(const CGsSpriteRect& other) const
	{
		float rx1 = std::max<float>(left, other.left);
		float rx2 = std::min<float>(right, other.right);
		float ry1 = std::max<float>(top, other.top);
		float ry2 = std::min<float>(bottom, other.bottom);

		if(rx1 < rx2 && ry1 < ry2)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	float left = 0;
	float top = 0;
	float right = 0;
	float bottom = 0;
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
			    (spriteRect.left == rect.left) &&
			    (spriteRect.right == rect.right))
			{
				//rect & spriteRect share the same horizontal edges
				spriteRect.top = std::min<float>(spriteRect.top, rect.top);
				spriteRect.bottom = std::max<float>(spriteRect.bottom, rect.bottom);
				return;
			}
			if(
			    (spriteRect.top == rect.top) &&
			    (spriteRect.bottom == rect.bottom))
			{
				//rect & spriteRect share the same vertical edges
				spriteRect.left = std::min<float>(spriteRect.left, rect.left);
				spriteRect.right = std::max<float>(spriteRect.right, rect.right);
				return;
			}
		}
		m_spriteRects.push_back(rect);
	}

private:
	std::vector<CGsSpriteRect> m_spriteRects;
};
