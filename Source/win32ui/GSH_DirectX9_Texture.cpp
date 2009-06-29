#include <assert.h>
#include "GSH_DirectX9.h"

LPDIRECT3DTEXTURE9 CGSH_DirectX9::LoadTexture(TEX0* pReg0, TEX1* pReg1, CLAMP* pClamp)
{
	LPDIRECT3DTEXTURE9 result(NULL);

	uint32 nPointer     = pReg0->GetBufPtr();
	uint32 nWidth		= pReg0->GetWidth();
	uint32 nHeight		= pReg0->GetHeight();

	//Set the right texture function
	//switch(pReg0->nFunction)
	//{
	//case 0:
	//	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	//	break;
	//case 1:
	//	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	//	break;
	//case 2:
	//case 3:
	//	//Just use modulate for now, but we'd need to use a different combining mode ideally
	//	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	//	break;
	//default:
	//	assert(0);
	//	break;
	//}

	//if(m_pProgram != NULL)
	//{
	//	if((pClamp->nWMS > 1) || (pClamp->nWMT > 1))
	//	{
	//		//We gotta use the shader

	//		glUseProgram(*m_pProgram);

	//		unsigned int nMode[2];
	//		unsigned int nMin[2];
	//		unsigned int nMax[2];

	//		nMode[0] = pClamp->nWMS - 1;
	//		nMode[1] = pClamp->nWMT - 1;

	//		nMin[0] = pClamp->GetMinU();
	//		nMin[1] = pClamp->GetMinV();

	//		nMax[0] = pClamp->GetMaxU();
	//		nMax[1] = pClamp->GetMaxV();

	//		for(unsigned int i = 0; i < 2; i++)
	//		{
	//			if(nMode[i] == 2)
	//			{
	//				//Check if we can transform in something more elegant

	//				for(unsigned int j = 1; j < 0x3FF; j = ((j << 1) | 1))
	//				{
	//					if(nMin[i] < j) break;
	//					if(nMin[i] != j) continue;

	//					if((nMin[i] & nMax[i]) != 0) break;

	//					nMin[i]++;
	//					nMode[i] = 3;
	//				}
	//			}
	//		}

	//		m_pProgram->SetUniformi("g_nClamp",	nMode[0],			nMode[1]);
	//		m_pProgram->SetUniformi("g_nMin",	nMin[0],			nMin[1]);
	//		m_pProgram->SetUniformi("g_nMax",	nMax[0],			nMax[1]);
	//		m_pProgram->SetUniformf("g_nSize",	(float)nWidth,		(float)nHeight);
	//	}
	//	else
	//	{
	//		glUseProgram(NULL);
	//	}
	//}

	result = TexCache_SearchLive(pReg0);
	if(result != NULL) return result;

	TEXA TexA;
	TexA <<= m_nReg[GS_REG_TEXA];

    uint32 textureChecksum = 0; 
	//switch(pReg0->nPsm)
	//{
	//case PSMT8:
	//	textureChecksum = ConvertTexturePsm8(pReg0, &TexA);
	//	break;
	//case PSMT4:
	//	textureChecksum = ConvertTexturePsm4(pReg0, &TexA);
	//	break;
	//case PSMT8H:
	//	textureChecksum = ConvertTexturePsm8H(pReg0, &TexA);
	//	break;
	//}

    if(textureChecksum)
    {
        //Check if we don't already have it somewhere...    
        result = TexCache_SearchDead(pReg0, textureChecksum);
        if(result != NULL)
        {
            return result;
        }
    }

	m_device->CreateTexture(nWidth, nHeight, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &result, NULL);

	switch(pReg0->nPsm)
	{
	case PSMCT32:
		TexUploader_Psm32(pReg0, &TexA, result);
		break;
	//case PSMCT16:
	//	((this)->*(m_pTexUploader_Psm16))(pReg0, &TexA);
	//	break;
	//case PSMCT16S:
	//	TexUploader_Psm16S_Hw(pReg0, &TexA);
	//	break;
	//case PSMT8:
	//	//((this)->*(m_pTexUploader_Psm8))(pReg0, &TexA);
	//	UploadTexturePsm8(pReg0, &TexA);
	//	break;
	//case PSMT4:
	//	//TexUploader_Psm4_Cvt(pReg0, &TexA);
	//	UploadTexturePsm8(pReg0, &TexA);
	//	break;
	//case PSMT4HL:
	//	TexUploader_Psm4H_Cvt<24>(pReg0, &TexA);
	//	break;
	//case PSMT4HH:
	//	TexUploader_Psm4H_Cvt<28>(pReg0, &TexA);
	//	break;
	//case PSMT8H:
	//	//TexUploader_Psm8H_Cvt(pReg0, &TexA);
	//	UploadTexturePsm8(pReg0, &TexA);
	//	break;
	//default:
	//	assert(0);
	//	break;
	}

//    DumpTexture(nWidth, nHeight);

	TexCache_Insert(pReg0, result, textureChecksum);

	return result;
}

void CGSH_DirectX9::TexUploader_Psm32(TEX0* pReg0, TEXA* pTexA, LPDIRECT3DTEXTURE9 texture)
{
	uint32 nPointer			= pReg0->GetBufPtr();
	unsigned int nWidth		= pReg0->GetWidth();
	unsigned int nHeight	= pReg0->GetHeight();
//	unsigned int nDstPitch	= nWidth;
//	uint32* pDst			= reinterpret_cast<uint32*>(m_pCvtBuffer);

	HRESULT result;
	D3DLOCKED_RECT rect;
	result = texture->LockRect(0, &rect, NULL, 0);

	uint32* pDst			= reinterpret_cast<uint32*>(rect.pBits);
	unsigned int nDstPitch	= rect.Pitch / 4;

	CPixelIndexorPSMCT32 Indexor(m_pRAM, nPointer, pReg0->nBufWidth);

	for(unsigned int j = 0; j < nHeight; j++)
	{
		for(unsigned int i = 0; i < nWidth; i++)
		{
			uint32 color = Indexor.GetPixel(i, j);
			pDst[i] = D3DCOLOR_ARGB(
				(color >> 24) & 0xFF, 
				(color >>  0) & 0xFF,
				(color >>  8) & 0xFF,
				(color >> 16) & 0xFF);
		}

		pDst += nDstPitch;
	}

	result = texture->UnlockRect(0);
	assert(result == S_OK);

	//glPixelTransferf(GL_ALPHA_SCALE, 2.0f);
	//glPixelStorei(GL_UNPACK_ROW_LENGTH, nWidth);
	//glTexImage2D(GL_TEXTURE_2D, 0, 4, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_pCvtBuffer);
}

/////////////////////////////////////////////////////////////
// Texture
/////////////////////////////////////////////////////////////

CGSH_DirectX9::CTexture::CTexture() :
m_nStart(0),
m_nSize(0),
m_nPSM(0),
m_nCLUTAddress(0),
m_nTex0(0),
m_nTexClut(0),
m_nIsCSM2(false),
m_texture(NULL),
m_checksum(0),
m_live(false)
{

}

CGSH_DirectX9::CTexture::~CTexture()
{
    Free();
}

void CGSH_DirectX9::CTexture::Free()
{
	if(m_texture != NULL)
	{
		m_texture->Release();
		m_texture = NULL;
        m_live = false;
	}
}

void CGSH_DirectX9::CTexture::InvalidateFromMemorySpace(uint32 nStart, uint32 nSize)
{
    if(!m_live) return;

	bool nInvalid = false;
	uint32 nEnd = nStart + nSize;
	uint32 nTexEnd = m_nStart + m_nSize;

	if((nStart >= m_nStart) && (nStart < nTexEnd))
	{
		nInvalid = true;
	}

	if((nEnd >= m_nStart) && (nEnd < nTexEnd))
	{
		nInvalid = true;
	}

	if(nInvalid)
	{
        m_live = false;
	}
}

/////////////////////////////////////////////////////////////
// Texture Caching
/////////////////////////////////////////////////////////////

LPDIRECT3DTEXTURE9 CGSH_DirectX9::TexCache_SearchLive(TEX0* pTex0)
{
    for(TextureList::iterator textureIterator(m_TexCache.begin());
        textureIterator != m_TexCache.end(); textureIterator++)
	{
        CTexture* texture = *textureIterator;
        if(!texture->m_live) continue;
        //if(!texture.IsValid()) continue;
        //if(m_TexCache[i].m_nStart != pTex0->GetBufPtr()) continue;
        //if(m_TexCache[i].m_nPSM != pTex0->nPsm) continue;
        //if(m_TexCache[i].m_nCLUTAddress != pTex0->GetCLUTPtr()) continue;
	    if(*(uint64*)pTex0 != texture->m_nTex0) continue;
	    if(texture->m_nIsCSM2)
	    {
		    if(((*(uint64*)GetTexClut()) & 0x3FFFFF) != texture->m_nTexClut) continue;
	    }
        m_TexCache.erase(textureIterator);
        m_TexCache.push_front(texture);
        return texture->m_texture;
	}

	return 0;
}

LPDIRECT3DTEXTURE9 CGSH_DirectX9::TexCache_SearchDead(TEX0* pTex0, uint32 checksum)
{
    for(TextureList::iterator textureIterator(m_TexCache.begin());
        textureIterator != m_TexCache.end(); textureIterator++)
    {
        CTexture* texture = *textureIterator;
        if(texture->m_texture == 0) continue;
        if(texture->m_checksum != checksum) continue;
        if(texture->m_nTex0 != *reinterpret_cast<uint64*>(pTex0)) continue;
        texture->m_live = true;
        m_TexCache.erase(textureIterator);
        m_TexCache.push_front(texture);
        return texture->m_texture;
    }

    return 0;
}

void CGSH_DirectX9::TexCache_Insert(TEX0* pTex0, LPDIRECT3DTEXTURE9 nTexture, uint32 checksum)
{
    CTexture* pTexture = *m_TexCache.rbegin();
    pTexture->Free();

	pTexture->m_nStart			= pTex0->GetBufPtr();
	pTexture->m_nSize			= pTex0->GetBufWidth() * pTex0->GetHeight() * GetPsmPixelSize(pTex0->nPsm) / 8;
//	pTexture->m_nPSM			= pTex0->nPsm;
//	pTexture->m_nCLUTAddress	= pTex0->GetCLUTPtr();
	pTexture->m_nTex0			= *reinterpret_cast<uint64*>(pTex0);
	pTexture->m_nTexClut		= (*(uint64*)GetTexClut()) & 0x3FFFFF;
	pTexture->m_nIsCSM2			= pTex0->nCSM == 1;
	pTexture->m_texture			= nTexture;
    pTexture->m_checksum        = checksum;
    pTexture->m_live            = true;

    m_TexCache.pop_back();
    m_TexCache.push_front(pTexture);
}

void CGSH_DirectX9::TexCache_InvalidateTextures(uint32 nStart, uint32 nSize)
{
    for(TextureList::iterator textureIterator(m_TexCache.begin());
        textureIterator != m_TexCache.end(); textureIterator++)
	{
        (*textureIterator)->InvalidateFromMemorySpace(nStart, nSize);
	}
}

void CGSH_DirectX9::TexCache_Flush()
{
    for(TextureList::iterator textureIterator(m_TexCache.begin());
        textureIterator != m_TexCache.end(); textureIterator++)
	{
        (*textureIterator)->Free();
	}
}
