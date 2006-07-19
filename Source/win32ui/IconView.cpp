#include "IconView.h"
#include <windows.h>
#include <gl/gl.h>
#include <math.h>

CIconView::CIconView(CIcon* pIcon)
{
	m_pIcon = pIcon;
	m_nTicks = 0;

	LoadTexture();

	m_nFrameLength = new unsigned int[m_pIcon->GetFrameCount()];

	for(unsigned int i = 0; i < m_pIcon->GetFrameCount(); i++)
	{
		const CIcon::FRAME* pFrame;
		pFrame = m_pIcon->GetFrame(i);

		if(pFrame->nKeyCount == 0) continue;

		m_nFrameLength[i] = (unsigned int)pFrame->pKeys[pFrame->nKeyCount - 1].nTime + 1;
	}

	m_nCurrentFrame = m_pIcon->GetFrameCount() - 1;
}

CIconView::~CIconView()
{
	glDeleteTextures(1, &m_nTexture);
	delete m_pIcon;
}

//static double m_nInter[42];

void CIconView::Render()
{
	unsigned int nVertexCount;
	const CIcon::VERTEX* pShape;
	const CIcon::TEXCOORD* pTexCoords;
	const CIcon::FRAME* pFrame;

	pFrame			= m_pIcon->GetFrame(0);
	pShape			= m_pIcon->GetShape(pFrame->nShapeId);
	pTexCoords		= m_pIcon->GetTexCoords();
	nVertexCount	= m_pIcon->GetVertexCount();

	m_nTicks++;
	m_nTicks %= (m_pIcon->GetFrameCount() * 7);

	glBindTexture(GL_TEXTURE_2D, m_nTexture);

	glBegin(GL_TRIANGLES);

	for(unsigned int i = 0; i < nVertexCount; i++)
	{
		glTexCoord2d(pTexCoords[i].nS, pTexCoords[i].nT);
		glVertex3d(pShape[i].nX, pShape[i].nY, pShape[i].nZ);
	}

	glEnd();

	glBindTexture(GL_TEXTURE_2D, NULL);
/*

	if(m_pIcon->GetFrameCount() == 1) return;

	unsigned int nVertexCount;
	const CIcon::VERTEX* pShape[2];
	const CIcon::FRAME* pFrame[2];
	const CIcon::TEXCOORD* pTexCoords;

	nVertexCount = m_pIcon->GetVertexCount();

	pFrame[0] = m_pIcon->GetFrame(m_nCurrentFrame);
	pFrame[1] = m_pIcon->GetFrame((m_nCurrentFrame + 1) % m_pIcon->GetFrameCount());

	//pFrame[0] = m_pIcon->GetFrame(0);
	//pFrame[1] = m_pIcon->GetFrame(1);

	pShape[0] = m_pIcon->GetShape(pFrame[0]->nShapeId);
	pShape[1] = m_pIcon->GetShape(pFrame[1]->nShapeId);

	pTexCoords = m_pIcon->GetTexCoords();

	double nMaxTime, nTime0, nTime1, nAmp0, nAmp1, nAlpha, nInt;

	nMaxTime = m_nFrameLength[m_nCurrentFrame];
	nTime0 = 0;
	nTime1 = m_nTicks % (unsigned int)nMaxTime;

	if(nTime1 == 14)
	{
		nTime1 = nTime1;
	}

	nAmp0 = 0;
	for(unsigned int i = 0; i < pFrame[0]->nKeyCount; i++)
	{
		nAmp1 = pFrame[0]->pKeys[i].nAmplitude;
		nInt = pFrame[0]->pKeys[i].nTime - nTime0;

		if(nTime1 <= pFrame[0]->pKeys[i].nTime) break;

		nTime0 = pFrame[0]->pKeys[i].nTime;
		nAmp0 = pFrame[0]->pKeys[i].nAmplitude;
	}

	nTime1 -= nTime0;

	nAlpha = ((double)(nAmp1 - nAmp0) / (double)(nInt));
	nAlpha = nAmp0 + (nTime1 * nAlpha);

//	m_nInter[m_nTicks % (unsigned int)nMaxTime] = nAlpha;

	glBindTexture(GL_TEXTURE_2D, m_nTexture);

	glBegin(GL_TRIANGLES);

	for(unsigned int i = 0; i < nVertexCount; i++)
	{
		double nX, nY, nZ;
		const CIcon::VERTEX* pV[2];

		glTexCoord2d(pTexCoords[i].nS, pTexCoords[i].nT);

		pV[0] = &pShape[0][i];
		pV[1] = &pShape[1][i];

		nX = pV[0]->nX + ((pV[1]->nX - pV[0]->nX) * nAlpha);
		nY = pV[0]->nY + ((pV[1]->nY - pV[0]->nY) * nAlpha);
		nZ = pV[0]->nZ + ((pV[1]->nZ - pV[0]->nZ) * nAlpha);

		glVertex3d(nX, nY, nZ);
	}

	glEnd();

	glBindTexture(GL_TEXTURE_2D, NULL);
	*/
}

void CIconView::Tick()
{
	m_nTicks++;
/*
	if(m_nTicks == m_nFrameLength[m_nCurrentFrame])
	{
		m_nTicks = 0;
		m_nCurrentFrame++;
		m_nCurrentFrame %= m_pIcon->GetFrameCount();
	}
*/
}

void CIconView::LoadTexture()
{
	uint32* pCvtBuffer;
	const uint16* pTexture;

	pCvtBuffer = new uint32[128 * 128];
	pTexture = m_pIcon->GetTexture();

	for(unsigned int i = 0; i < (128 * 128); i++)
	{
		uint8 nR, nG, nB, nA;
		uint16 nPixel;

		nPixel = pTexture[i];

		nR = ((nPixel & 0x001F) >>  0) << 3;
		nG = ((nPixel & 0x03E0) >>  5) << 3;
		nB = ((nPixel & 0x7C00) >> 10) << 3;
		nA = (nPixel & 0x8000) != 0 ? 0xFF : 0x00;

		((uint8*)&pCvtBuffer[i])[0] = nR;
		((uint8*)&pCvtBuffer[i])[1] = nG;
		((uint8*)&pCvtBuffer[i])[2] = nB;
		((uint8*)&pCvtBuffer[i])[3] = nA;
	}

	glGenTextures(1, &m_nTexture);

	glBindTexture(GL_TEXTURE_2D, m_nTexture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glTexImage2D(GL_TEXTURE_2D, 0, 4, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, pCvtBuffer);
	
	glBindTexture(GL_TEXTURE_2D, NULL);

	delete [] pCvtBuffer;
}
