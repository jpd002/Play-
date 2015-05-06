#include "IconMesh.h"
#include <gl/glew.h>
#include <math.h>

CIconMesh::CIconMesh(const IconPtr& icon)
: m_icon(icon)
, m_time(0)
, m_texture(0)
, m_animLength(0)
{
	m_frameInfluences.reserve(m_icon->GetFrameCount());

	//Compute animation length
	for(unsigned int i = 0; i < m_icon->GetFrameCount(); i++)
	{
		const CIcon::FRAME* frame = m_icon->GetFrame(i);
		for(unsigned int j = 0; j < frame->nKeyCount; j++)
		{
			const CIcon::KEY& key = frame->pKeys[j];
			m_animLength = std::max<float>(m_animLength, key.nTime);
		}
	}

	LoadTexture();
	ComputeFrameInfluences();
}

CIconMesh::~CIconMesh()
{
	glDeleteTextures(1, &m_texture);
}

void CIconMesh::Render() const
{
	const CIcon::TEXCOORD* pTexCoords	= m_icon->GetTexCoords();
	unsigned int nVertexCount			= m_icon->GetVertexCount();

	glBindTexture(GL_TEXTURE_2D, m_texture);

	glBegin(GL_TRIANGLES);

	for(unsigned int i = 0; i < nVertexCount; i++)
	{
		glTexCoord2f(pTexCoords[i].nS, pTexCoords[i].nT);

		float totalX = 0;
		float totalY = 0;
		float totalZ = 0;

		for(unsigned int j = 0; j < m_frameInfluences.size(); j++)
		{
			const FRAMEINFLUENCE& influence(m_frameInfluences[j]);
			const CIcon::VERTEX* pShape = m_icon->GetShape(influence.shapeId);
			totalX += pShape[i].nX * influence.amplitude;
			totalY += pShape[i].nY * influence.amplitude;
			totalZ += pShape[i].nZ * influence.amplitude;
		}

		glVertex3f(totalX, totalY, totalZ);
	}

	glEnd();

	glBindTexture(GL_TEXTURE_2D, NULL);
}

void CIconMesh::Update(float dt)
{
	ComputeFrameInfluences();

	m_time += (dt * 133);
	if(m_time > m_animLength)
	{
		m_time -= m_animLength;
	}
}

void CIconMesh::ComputeFrameInfluences()
{
	m_frameInfluences.resize(0);

	for(unsigned int i = 0; i < m_icon->GetFrameCount(); i++)
	{
		//We need to check if this frame influences the mesh at the current time

		const CIcon::FRAME* frame = m_icon->GetFrame(i);
		float frameAmp = 0;

		for(unsigned int j = 0; j < frame->nKeyCount; j++)
		{
			const CIcon::KEY& key = frame->pKeys[j];
			//Check end and start cases
			if(
				((j == 0) && m_time <= key.nTime) ||
				((j == frame->nKeyCount - 1))
				)
			{
				frameAmp = key.nAmplitude;
				break;
			}

			assert((j + 1) != frame->nKeyCount);
			const CIcon::KEY& key0 = frame->pKeys[j + 0];
			const CIcon::KEY& key1 = frame->pKeys[j + 1];

			if(m_time >= key0.nTime && m_time <= key1.nTime)
			{
				float insideTime = m_time - key0.nTime;
				float slope = (key1.nAmplitude - key0.nAmplitude) / (key1.nTime - key0.nTime);
				frameAmp = (insideTime * slope) + key0.nAmplitude;
				break;
			}
		}

		if(frameAmp != 0)
		{
			FRAMEINFLUENCE influence;
			influence.shapeId = frame->nShapeId;
			influence.amplitude = frameAmp;
			m_frameInfluences.push_back(influence);
		}
	}

	if(
		(m_frameInfluences.size() == 0) && 
		(m_icon->GetFrameCount() == 1))
	{
		const CIcon::FRAME* frame = m_icon->GetFrame(0);

		FRAMEINFLUENCE influence;
		influence.shapeId = frame->nShapeId;
		influence.amplitude = 1;
		m_frameInfluences.push_back(influence);
	}
}

void CIconMesh::LoadTexture()
{
	uint32* pCvtBuffer = new uint32[128 * 128];
	const uint16* pTexture = m_icon->GetTexture();

	for(unsigned int i = 0; i < (128 * 128); i++)
	{
		uint16 nPixel = pTexture[i];

		uint8 nR = ((nPixel & 0x001F) >>  0) << 3;
		uint8 nG = ((nPixel & 0x03E0) >>  5) << 3;
		uint8 nB = ((nPixel & 0x7C00) >> 10) << 3;
		uint8 nA = (nPixel & 0x8000) != 0 ? 0xFF : 0x00;

		((uint8*)&pCvtBuffer[i])[0] = nR;
		((uint8*)&pCvtBuffer[i])[1] = nG;
		((uint8*)&pCvtBuffer[i])[2] = nB;
		((uint8*)&pCvtBuffer[i])[3] = nA;
	}

	glGenTextures(1, &m_texture);

	glBindTexture(GL_TEXTURE_2D, m_texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glTexImage2D(GL_TEXTURE_2D, 0, 4, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, pCvtBuffer);
	
	glBindTexture(GL_TEXTURE_2D, NULL);

	delete [] pCvtBuffer;
}
