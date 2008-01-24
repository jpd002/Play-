#ifndef _GSH_SOFTWARE_H_
#define _GSH_SOFTWARE_H_

#include "../GSHandler.h"
#include "../PS2VM.h"
#include "win32/Window.h"
#include <ddraw.h>

class CGSH_Software : public CGSHandler
{
public:
                                    CGSH_Software(Framework::Win32::CWindow*);
    virtual                         ~CGSH_Software();

    static FactoryFunction          GetFactoryFunction(Framework::Win32::CWindow*);

private:
   	static CGSHandler*              GSHandlerFactory(Framework::Win32::CWindow*);

    virtual void                    InitializeImpl();
    virtual void                    UpdateViewportImpl();
    virtual void                    FlipImpl();
    virtual void                    ProcessImageTransfer(uint32, uint32);

    void                            RecreateFrameBuffer(unsigned int, unsigned int);

    LPDIRECTDRAW                    m_pDirectDraw;
    LPDIRECTDRAWSURFACE             m_pPrimarySurface;
    LPDIRECTDRAWSURFACE             m_pFrameBufferSurface;
    LPDIRECTDRAWCLIPPER             m_pClipper;

    unsigned int                    m_nWidth;
    unsigned int                    m_nHeight;

    Framework::Win32::CWindow*      m_pOutputWnd;
    uint32*                         m_pFrameBuffer;
};

#endif
