#include "GSH_Software.h"
#include "PtrMacro.h"

using namespace Framework;
using namespace std;
using namespace std::tr1;

CGSH_Software::CGSH_Software(Win32::CWindow* pOutputWnd) :
m_pOutputWnd(pOutputWnd),
m_pDirectDraw(NULL),
m_pPrimarySurface(NULL),
m_pFrameBufferSurface(NULL),
m_pClipper(NULL),
m_pFrameBuffer(NULL),
m_nWidth(0),
m_nHeight(0)
{
//    if(FAILED(DirectDrawCreate(NULL, &m_pDirectDraw, NULL)))
    {
        throw runtime_error("Couldn't create DirectDraw object.");
    }

    if(FAILED(m_pDirectDraw->SetCooperativeLevel(m_pOutputWnd->m_hWnd, DDSCL_NORMAL | DDSCL_MULTITHREADED)))
    {
        throw runtime_error("Couldn't SetCooperativeLevel.");
    }

    {
        DDSURFACEDESC ddsd;
        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize             = sizeof(ddsd);
        ddsd.dwFlags            = DDSD_CAPS;
        ddsd.ddsCaps.dwCaps     = DDSCAPS_PRIMARYSURFACE;

        if(FAILED(m_pDirectDraw->CreateSurface(&ddsd, &m_pPrimarySurface, NULL)))
        {
            throw runtime_error("Couldn't create primary surface.");
        }
    }

    m_pDirectDraw->CreateClipper(NULL, &m_pClipper, NULL);
    m_pClipper->SetHWnd(NULL, m_pOutputWnd->m_hWnd);
    m_pPrimarySurface->SetClipper(m_pClipper);

    RecreateFrameBuffer(512, 384);
}

CGSH_Software::~CGSH_Software()
{
    DELETEPTR(m_pFrameBuffer);
    FREECOM(m_pClipper);
    FREECOM(m_pFrameBufferSurface);
    FREECOM(m_pPrimarySurface);
    FREECOM(m_pDirectDraw);
}

void CGSH_Software::InitializeImpl()
{

}

void CGSH_Software::ReleaseImpl()
{

}

CGSHandler::FactoryFunction CGSH_Software::GetFactoryFunction(Win32::CWindow* pOutputWnd)
{
    return bind(&CGSH_Software::GSHandlerFactory, pOutputWnd);
}

CGSHandler* CGSH_Software::GSHandlerFactory(Win32::CWindow* pOutputWnd)
{
    return new CGSH_Software(pOutputWnd);
}

void CGSH_Software::UpdateViewportImpl()
{
	RecreateFrameBuffer(GetCrtWidth(), GetCrtHeight());
}

void CGSH_Software::FlipImpl()
{
    DISPFB* pDispFb(GetDispFb(1));

    FetchImagePSMCT32(m_pFrameBuffer, pDispFb->nBufPtr, pDispFb->nBufWidth, m_nWidth, m_nHeight);

    {
        DDSURFACEDESC ddsd;
        memset(&ddsd, 0, sizeof(DDSURFACEDESC));
        ddsd.dwSize = sizeof(DDSURFACEDESC);

        m_pFrameBufferSurface->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);

        if(ddsd.lpSurface != NULL)
        {
            uint32* pDst(reinterpret_cast<uint32*>(ddsd.lpSurface));
            uint32* pSrc(m_pFrameBuffer);
            ddsd.lPitch /= sizeof(uint32);

            for(unsigned int i = 0; i < m_nHeight; i++)
            {
                for(unsigned int j = 0; j < m_nWidth; j++)
                {
                    uint8* pColor(reinterpret_cast<uint8*>(&pSrc[j]));
                    pDst[j] = RGB(pColor[2], pColor[1], pColor[0]);
                }
//                memcpy(pDst, pSrc, m_nWidth * 4);
                pDst += ddsd.lPitch;
                pSrc += m_nWidth;
            }
        }

        m_pFrameBufferSurface->Unlock(NULL);
    }

    RECT Rect;
    SetRect(&Rect, 0, 0, m_nWidth, m_nHeight);
    ClientToScreen(m_pOutputWnd->m_hWnd, reinterpret_cast<POINT*>(&Rect) + 0);
    ClientToScreen(m_pOutputWnd->m_hWnd, reinterpret_cast<POINT*>(&Rect) + 1);
    m_pPrimarySurface->Blt(&Rect, m_pFrameBufferSurface, NULL, DDBLT_WAIT, NULL);

	CGSHandler::FlipImpl();
}

void CGSH_Software::ProcessImageTransfer(uint32 nAddress, uint32 nLength)
{

}

void CGSH_Software::ReadFramebuffer(uint32, uint32, void*)
{

}

void CGSH_Software::RecreateFrameBuffer(unsigned int nWidth, unsigned int nHeight)
{
    if((nWidth == m_nWidth) && (nHeight == m_nHeight)) return;

    m_nWidth = nWidth;
    m_nHeight = nHeight;

    DELETEPTR(m_pFrameBuffer);
    FREECOM(m_pFrameBufferSurface);

    DDSURFACEDESC ddsd;
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize             = sizeof(ddsd);
    ddsd.dwFlags            = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps     = DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwWidth            = m_nWidth;
    ddsd.dwHeight           = m_nHeight;

    if(FAILED(m_pDirectDraw->CreateSurface(&ddsd, &m_pFrameBufferSurface, NULL)))
    {
        throw runtime_error("Couldn't create frame buffer surface.");
    }

    m_pFrameBuffer = new uint32[m_nWidth * m_nHeight];

    RECT rc;
	SetRect(&rc, 0, 0, m_nWidth, m_nHeight);
	AdjustWindowRect(&rc, GetWindowLong(m_pOutputWnd->m_hWnd, GWL_STYLE), FALSE);
	m_pOutputWnd->SetSize((rc.right - rc.left), (rc.bottom - rc.top));
}
