#pragma once

#include <QWidget>
#include <QWindow>
#include <QComboBox>
#include <QPushButton>

#include "gs/GSHandler.h"
#include "FrameDump.h"
#include "PixelBufferView.h"

class CGsContextView : public QWidget
{
public:
	typedef std::unique_ptr<CGSHandler> GsHandlerPtr;

	enum FB_DISPLAY_MODE
	{
		FB_DISPLAY_MODE_RAW,
		FB_DISPLAY_MODE_448P,
		FB_DISPLAY_MODE_448I
	};

	CGsContextView(QWidget*, QComboBox*, QPushButton*, QPushButton*, const GsHandlerPtr&, unsigned int);
	virtual ~CGsContextView() = default;

	void SetFbDisplayMode(FB_DISPLAY_MODE);

	void UpdateState(CGsPacketMetadata*, DRAWINGKICK_INFO*);
	void SetSelection(int);

private:
	typedef std::array<uint32, 256> ColorArray;

	void UpdateBufferView();
	void UpdateFramebufferView();
	void RenderDrawKick(Framework::CBitmap&);

	static uint32 Color_Ps2ToRGBA(uint32);
	static void BrightenBitmap(Framework::CBitmap&);
	static Framework::CBitmap LookupBitmap(const Framework::CBitmap&, const ColorArray&);
	static Framework::CBitmap ExtractAlpha32(const Framework::CBitmap&);

	unsigned int m_contextId = 0;
	std::unique_ptr<CPixelBufferView> m_bufferView;
	const GsHandlerPtr& m_gs;
	FB_DISPLAY_MODE m_fbDisplayMode = FB_DISPLAY_MODE_RAW;
	DRAWINGKICK_INFO m_drawingKick;
	int m_selected = 0;
};
