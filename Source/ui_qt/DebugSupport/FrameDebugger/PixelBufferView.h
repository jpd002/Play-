#pragma once

#include <QWidget>
#include <QComboBox>
#include <QPaintEvent>
#include <QWheelEvent>

#include "bitmap/Bitmap.h"
#include <memory>
#include <vector>
#include "GSH_OpenGLQt_Framedebugger.h"

class CPixelBufferView : public QWidget
{
	Q_OBJECT	
public:
	typedef std::pair<std::string, Framework::CBitmap> PixelBuffer;
	typedef std::vector<PixelBuffer> PixelBufferArray;

	CPixelBufferView(QWidget*, QComboBox*);
	virtual ~CPixelBufferView() = default;

	void SetPixelBuffers(PixelBufferArray);

	void FitBitmap();
	void OnSaveBitmap();

protected:
	void showEvent(QShowEvent*) Q_DECL_OVERRIDE;
	void paintEvent(QPaintEvent*) Q_DECL_OVERRIDE;

protected slots:
	void Refresh();
	void OnMousePressEvent(QMouseEvent*);
	void OnMouseMoveEvent(QMouseEvent*);
	void OnMouseWheelEvent(QWheelEvent*);

private:
	struct VERTEX
	{
		float position[3];
		float texCoord[2];
	};

	QComboBox* m_contextBuffer;
	const PixelBuffer* GetSelectedPixelBuffer();
	void CreateSelectedPixelBufferTexture();

	static Framework::CBitmap ConvertBGRToRGB(Framework::CBitmap);

	void DrawCheckerboard();
	void DrawPixelBuffer();

	PixelBufferArray m_pixelBuffers;

	float m_panX;
	float m_panY;
	float m_zoomFactor;

	bool m_dragging;
	int m_dragBaseX;
	int m_dragBaseY;

	float m_panXDragBase;
	float m_panYDragBase;

	std::unique_ptr<CGSH_OpenGLFramedebugger> m_gs;
	QWindow* m_openglpanel = nullptr;
};
