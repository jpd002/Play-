#include "PixelBufferView.h"
#include <assert.h>
#include <vector>
#include <QWheelEvent>
#include <algorithm>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QMessageBox>

#include "bitmap/BMP.h"
#include "StdStreamUtils.h"
#include "string_format.h"
#include "../../openglwindow.h"

CPixelBufferView::CPixelBufferView(QWidget* parent, QComboBox* contextBuffer)
    : QWidget(parent)
    , m_contextBuffer(contextBuffer)
    , m_zoomFactor(1)
    , m_panX(0)
    , m_panY(0)
    , m_dragging(false)
    , m_dragBaseX(0)
    , m_dragBaseY(0)
    , m_panXDragBase(0)
    , m_panYDragBase(0)
{
	m_openglpanel = new OpenGLWindow;
	auto container = QWidget::createWindowContainer(m_openglpanel, this);
	container->sizePolicy().setHorizontalStretch(3);
	container->sizePolicy().setHorizontalPolicy(QSizePolicy::Expanding);
	
	auto layout = new QVBoxLayout;
	setLayout(layout);
	layout->addWidget(container);
	
	m_openglpanel->create();
	m_gs = std::make_unique<CGSH_OpenGLFramedebugger>(m_openglpanel);
	m_gs->InitializeImpl();
	m_gs->PrepareFramedebugger();
	
	connect(m_openglpanel, &QWindow::widthChanged, this, &CPixelBufferView::Refresh);
	connect(m_openglpanel, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(OnMouseMoveEvent(QMouseEvent*)));
	connect(m_openglpanel, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(OnMousePressEvent(QMouseEvent*)));
	connect(m_openglpanel, SIGNAL(mouseWheel(QWheelEvent*)), this, SLOT(OnMouseWheelEvent(QWheelEvent*)));
}

void CPixelBufferView::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);

	CHECKGLERROR();
	Refresh();
}

void CPixelBufferView::paintEvent(QPaintEvent* event)
{
	QWidget::paintEvent(event);
	Refresh();
}

void CPixelBufferView::SetPixelBuffers(PixelBufferArray pixelBuffers)
{
	m_pixelBuffers = std::move(pixelBuffers);
	//Update buffer titles
	{
		//Save previously selected index
		int selectedIndex = m_contextBuffer->currentIndex();
		m_contextBuffer->blockSignals(true);
		m_contextBuffer->clear();

		auto titleCount = 0;
		for(const auto& pixelBuffer : m_pixelBuffers)
		{
			m_contextBuffer->addItem(pixelBuffer.first.c_str());
			++titleCount;
		}

		//Restore selected index
		if((selectedIndex != -1) && (selectedIndex < titleCount))
		{
			m_contextBuffer->setCurrentIndex(selectedIndex);
		}
		m_contextBuffer->blockSignals(false);
	}
	CreateSelectedPixelBufferTexture();
	Refresh();
}

void CPixelBufferView::Refresh()
{
	m_gs->Begin();
	{
		DrawCheckerboard();
		DrawPixelBuffer();
	}
	m_gs->PresentBackbuffer();
}

void CPixelBufferView::DrawCheckerboard()
{
	auto clientRect = m_openglpanel->size();
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
	clientRect *= devicePixelRatioF();
#endif
	float screenSizeVector[2] =
	    {
	        static_cast<float>(clientRect.width()),
	        static_cast<float>(clientRect.height())};
	m_gs->DrawCheckerboard(screenSizeVector);
}

void CPixelBufferView::DrawPixelBuffer()
{
	auto pixelBuffer = GetSelectedPixelBuffer();
	if(!pixelBuffer) return;
	const auto& pixelBufferBitmap = pixelBuffer->second;

	auto clientRect = m_openglpanel->size();

	float screenSizeVector[2] =
	    {
	        static_cast<float>(clientRect.width()),
	        static_cast<float>(clientRect.height())};

	float bufferSizeVector[2] =
	    {
	        static_cast<float>(pixelBufferBitmap.GetWidth()),
	        static_cast<float>(pixelBufferBitmap.GetHeight())};

	m_gs->DrawPixelBuffer(screenSizeVector, bufferSizeVector, m_panX, m_panY, m_zoomFactor);
}

void CPixelBufferView::OnMousePressEvent(QMouseEvent* event)
{
	if(event->buttons() & Qt::LeftButton)
	{
		auto mousePoint = event->localPos();
		m_dragBaseX = mousePoint.x();
		m_dragBaseY = mousePoint.y();
		m_panXDragBase = m_panX;
		m_panYDragBase = m_panY;
	}
}

void CPixelBufferView::OnMouseMoveEvent(QMouseEvent* event)
{
	if(event->buttons() & Qt::LeftButton)
	{
		auto clientRect = m_openglpanel->size();
		auto mousePoint = event->localPos();
		m_panX = m_panXDragBase + (static_cast<float>(mousePoint.x() - m_dragBaseX) / static_cast<float>(clientRect.width() / 2)) / m_zoomFactor;
		m_panY = m_panYDragBase - (static_cast<float>(mousePoint.y() - m_dragBaseY) / static_cast<float>(clientRect.height() / 2)) / m_zoomFactor;
		Refresh();
	}
}

void CPixelBufferView::OnMouseWheelEvent(QWheelEvent* event)
{
	float newZoom = 0;
	auto z = event->delta();
	if(z <= -1)
	{
		newZoom = m_zoomFactor / 2;
	}
	else if(z >= 1)
	{
		newZoom = m_zoomFactor * 2;
	}

	if(newZoom != 0)
	{
		auto clientRect = m_openglpanel->size();
		auto mousePoint = event->posF();
		float relPosX = static_cast<float>(mousePoint.x()) / static_cast<float>(clientRect.width());
		float relPosY = static_cast<float>(mousePoint.y()) / static_cast<float>(clientRect.height());
		relPosX = std::max(relPosX, 0.f);
		relPosX = std::min(relPosX, 1.f);
		relPosY = std::max(relPosY, 0.f);
		relPosY = std::min(relPosY, 1.f);

		relPosX = (relPosX - 0.5f) * 2;
		relPosY = (relPosY - 0.5f) * 2;

		float panModX = (1 - newZoom / m_zoomFactor) * relPosX;
		float panModY = (1 - newZoom / m_zoomFactor) * relPosY;
		m_panX += panModX;
		m_panY -= panModY;

		m_zoomFactor = newZoom;
		Refresh();
	}
}

const CPixelBufferView::PixelBuffer* CPixelBufferView::GetSelectedPixelBuffer()
{
	if(m_pixelBuffers.empty()) return nullptr;

	int selectedPixelBufferIndex = m_contextBuffer->currentIndex();
	if(selectedPixelBufferIndex < 0) return nullptr;

	assert(selectedPixelBufferIndex < m_pixelBuffers.size());
	if(selectedPixelBufferIndex >= m_pixelBuffers.size()) return nullptr;

	return &m_pixelBuffers[selectedPixelBufferIndex];
}

void CPixelBufferView::CreateSelectedPixelBufferTexture()
{
	auto pixelBuffer = GetSelectedPixelBuffer();
	if(!pixelBuffer || !m_gs) return;

	m_gs->Begin();
	m_gs->LoadTextureFromBitmap(pixelBuffer->second);
}

void CPixelBufferView::OnSaveBitmap()
{
	auto pixelBuffer = GetSelectedPixelBuffer();
	if(!pixelBuffer) return;
	auto pixelBufferBitmap = pixelBuffer->second;

	auto bpp = pixelBufferBitmap.GetBitsPerPixel();
	if((bpp == 24) || (bpp == 32))
	{
		pixelBufferBitmap = ConvertBGRToRGB(std::move(pixelBufferBitmap));
	}

	QFileDialog dialog(this);
	dialog.setAcceptMode(QFileDialog::AcceptSave);
	dialog.setFileMode(QFileDialog::AnyFile);
	dialog.setNameFilter(tr("Bitmap Image (*.bmp)"));
	if(dialog.exec())
	{
		auto filePath = dialog.selectedFiles().first();
		try
		{
			auto outputStream = Framework::CreateOutputStdStream(filePath.toStdString());
			Framework::CBMP::WriteBitmap(pixelBufferBitmap, outputStream);
		}
		catch(const std::exception& exception)
		{
			auto message = string_format("Failed to save buffer to file:\r\n\r\n%s", exception.what());
			QMessageBox msgBox;
			msgBox.setText(message.c_str());
			msgBox.exec();
		}
	}
}

Framework::CBitmap CPixelBufferView::ConvertBGRToRGB(Framework::CBitmap bitmap)
{
	for(int32 y = 0; y < bitmap.GetHeight(); y++)
	{
		for(int32 x = 0; x < bitmap.GetWidth(); x++)
		{
			auto color = bitmap.GetPixel(x, y);
			std::swap(color.r, color.b);
			bitmap.SetPixel(x, y, color);
		}
	}
	return std::move(bitmap);
}

void CPixelBufferView::FitBitmap()
{
	auto pixelBuffer = GetSelectedPixelBuffer();
	if(!pixelBuffer) return;
	const auto& pixelBufferBitmap = pixelBuffer->second;

	auto clientRect = m_openglpanel->size();
	unsigned int marginSize = 50;
	float normalizedSizeX = static_cast<float>(pixelBufferBitmap.GetWidth()) / static_cast<float>(clientRect.width() - marginSize);
	float normalizedSizeY = static_cast<float>(pixelBufferBitmap.GetHeight()) / static_cast<float>(clientRect.height() - marginSize);

	float normalizedSize = std::max<float>(normalizedSizeX, normalizedSizeY);

	m_zoomFactor = 1 / normalizedSize;
	m_panX = 0;
	m_panY = 0;

	Refresh();
}
