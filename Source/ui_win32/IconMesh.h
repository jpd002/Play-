#ifndef _ICONMESH_H_
#define _ICONMESH_H_

#include "../saves/Icon.h"

class CIconMesh
{
public:
					CIconMesh(const IconPtr&);
	virtual			~CIconMesh();

	void			Render() const;
	void			Update(float);

private:
	struct FRAMEINFLUENCE
	{
		unsigned int	shapeId;
		float			amplitude;
	};

	typedef std::vector<FRAMEINFLUENCE> FrameInfluenceArray;

	void					ComputeFrameInfluences();
	void					LoadTexture();

	unsigned int			m_texture;
	IconPtr					m_icon;

	float					m_animLength;
	float					m_time;

	FrameInfluenceArray		m_frameInfluences;
};

typedef std::tr1::shared_ptr<CIconMesh> IconMeshPtr;

#endif
