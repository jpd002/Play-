#ifndef _GSH_DIRECTX9_H_
#define _GSH_DIRECTX9_H_

#include "../GSHandler.h"
#include "win32/Window.h"
#include "OutputWnd.h"
#ifdef _DEBUG
#define D3D_DEBUG_INFO
#endif
#include <d3d9.h>

class CGSH_DirectX9 : public CGSHandler
{
public:
									CGSH_DirectX9(Framework::Win32::CWindow*);
	virtual                         ~CGSH_DirectX9();

	void							ProcessImageTransfer(uint32, uint32);

    static FactoryFunction          GetFactoryFunction(Framework::Win32::CWindow*);

    virtual void                    InitializeImpl();
    virtual void                    ReleaseImpl();
	virtual void					FlipImpl();

protected:
	virtual void					WriteRegisterImpl(uint8, uint64);

private:
	struct VERTEX
	{
		uint64						nPosition;
		uint64						nRGBAQ;
		uint64						nUV;
		uint64						nST;
		uint8						nFog;
	};

	void							BeginScene();
	void							EndScene();
	void							ReCreateDevice(int, int);

	virtual void                    SetViewport(int, int);
	void							SetReadCircuitMatrix(int, int);
	void							UpdateViewportImpl();
	void							VertexKick(uint8, uint64);

	void							SetRenderingContext(unsigned int);

	void							Prim_Triangle();

    static CGSHandler*				GSHandlerFactory(Framework::Win32::CWindow*);

	COutputWnd*						m_pOutputWnd;

	//Context variables (put this in a struct or something?)
	float							m_nPrimOfsX;
	float							m_nPrimOfsY;

	PRMODE							m_PrimitiveMode;
	unsigned int					m_nPrimitiveType;

	VERTEX							m_VtxBuffer[3];
	int								m_nVtxCount;

	int								m_nWidth;
	int								m_nHeight;
	LPDIRECT3D9						m_d3d;
	LPDIRECT3DDEVICE9				m_device;

	LPDIRECT3DVERTEXBUFFER9			m_triangleVb;
	bool							m_sceneBegun;
};

#endif
