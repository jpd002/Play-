#ifndef _ICONVIEW_H_
#define _ICONVIEW_H_

#include "../saves/Icon.h"

class CIconView
{
public:
					CIconView(CIcon*);
					~CIconView();

	void			Render();
	void			Tick();

private:
	void			LoadTexture();

	unsigned int	m_nTicks;
	unsigned int	m_nTexture;
	unsigned int*	m_nFrameLength;
	unsigned int	m_nCurrentFrame;
	CIcon*			m_pIcon;
};

#endif
