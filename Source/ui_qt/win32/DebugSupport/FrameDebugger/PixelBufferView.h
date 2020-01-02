#pragma once

#include "win32/Window.h"
#include "bitmap/Bitmap.h"
#include "PixelBufferViewOverlay.h"
#include <memory>
#include <vector>
#include "GSH_OpenGL_Framedebugger.h"
#include "../OutputWnd.h"

class CPixelBufferView : public Framework::Win32::CWindow
{
public:
	typedef std::pair<std::string, Framework::CBitmap> PixelBuffer;
	typedef std::vector<PixelBuffer> PixelBufferArray;

	CPixelBufferView(HWND, const RECT&);
	virtual ~CPixelBufferView() = default;

	void SetPixelBuffers(PixelBufferArray);

	void FitBitmap();
	bool m_init = false;
protected:
	void Refresh();
	long OnPaint() override;
	long OnEraseBkgnd() override;


	long OnCommand(unsigned short, unsigned short, HWND) override;
	long OnSize(unsigned int, unsigned int, unsigned int) override;

	long OnLeftButtonDown(int, int) override;
	long OnLeftButtonUp(int, int) override;

	long OnMouseMove(WPARAM, int, int) override;

	long OnMouseWheel(int, int, short) override;


private:

	struct VERTEX
	{
		float position[3];
		float texCoord[2];
	};

	const PixelBuffer* GetSelectedPixelBuffer();
	void CreateSelectedPixelBufferTexture();

	void OnSaveBitmap();
	static Framework::CBitmap ConvertBGRToRGB(Framework::CBitmap);

	void DrawCheckerboard();
	void DrawPixelBuffer();


	PixelBufferArray m_pixelBuffers;

	std::unique_ptr<CPixelBufferViewOverlay> m_overlay;

	float m_panX;
	float m_panY;
	float m_zoomFactor;

	bool m_dragging;
	int m_dragBaseX;
	int m_dragBaseY;

	float m_panXDragBase;
	float m_panYDragBase;

	std::unique_ptr<CGSH_OpenGLFramedebugger> m_gs;
	std::unique_ptr<COutputWnd> m_handlerOutputWindow;

};
