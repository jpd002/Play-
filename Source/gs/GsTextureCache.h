#pragma once

#include <list>
#include "GSHandler.h"
#include "GsCachedArea.h"

#define TEX0_CLUTINFO_MASK (~0xFFFFFFE000000000ULL)

template <typename TextureHandleType>
class CGsTextureCache
{
public:
	class CTexture
	{
	public:
		void Reset()
		{
			m_live = false;
			m_textureHandle = TextureHandleType();
		}

		uint64        m_tex0 = 0;
		bool          m_live = false;
		CGsCachedArea m_cachedArea;

		//Platform specific
		TextureHandleType m_textureHandle;
	};

	enum
	{
		MAX_TEXTURE_CACHE = 256,
	};

	CGsTextureCache()
	{
		for(unsigned int i = 0; i < MAX_TEXTURE_CACHE; i++)
		{
			m_textureCache.push_back(std::make_shared<CTexture>());
		}
	}

	CTexture* Search(const CGSHandler::TEX0& tex0)
	{
		uint64 maskedTex0 = static_cast<uint64>(tex0) & TEX0_CLUTINFO_MASK;

		for(auto textureIterator(m_textureCache.begin());
			textureIterator != m_textureCache.end(); textureIterator++)
		{
			auto texture = *textureIterator;
			if(!texture->m_live) continue;
			if(maskedTex0 != texture->m_tex0) continue;
			m_textureCache.erase(textureIterator);
			m_textureCache.push_front(texture);
			return texture.get();
		}

		return nullptr;
	}

	void Insert(const CGSHandler::TEX0& tex0, TextureHandleType textureHandle)
	{
		auto texture = *m_textureCache.rbegin();
		texture->Reset();

		texture->m_cachedArea.SetArea(tex0.nPsm, tex0.GetBufPtr(), tex0.GetBufWidth(), tex0.GetHeight());

		texture->m_tex0          = static_cast<uint64>(tex0) & TEX0_CLUTINFO_MASK;
		texture->m_textureHandle = std::move(textureHandle);
		texture->m_live          = true;

		m_textureCache.pop_back();
		m_textureCache.push_front(texture);
	}

	void InvalidateRange(uint32 start, uint32 size)
	{
		std::for_each(std::begin(m_textureCache), std::end(m_textureCache), 
			[start, size] (TexturePtr& texture) { if(texture->m_live) { texture->m_cachedArea.Invalidate(start, size); } });
	}

	void Flush()
	{
		std::for_each(std::begin(m_textureCache), std::end(m_textureCache), 
			[] (TexturePtr& texture) { texture->Reset(); });
	}

private:
	typedef std::shared_ptr<CTexture> TexturePtr;
	typedef std::list<TexturePtr> TextureList;

	TextureList    m_textureCache;
};
