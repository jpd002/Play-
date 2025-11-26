#include "vulkanwindow.h"
#include "macos/LayerFromView.h"

VulkanWindow::VulkanWindow(QWindow* parent)
    : OutputWindow(parent)
{
#ifdef __APPLE__
	//Qt6's MetalLayer has a transaction system that will cause annoying
	//hangs when resizing or changing the window's visibility.
	//https://github.com/qt/qtbase/blob/master/src/plugins/platforms/cocoa/qnsview_drawing.mm#L104
	setenv("QT_MTL_NO_TRANSACTION", "1", true);
#endif
	setSurfaceType(QWindow::VulkanSurface);
#ifdef __APPLE__
	m_metalLayer = GetLayerFromView(reinterpret_cast<void*>(winId()));
#endif
}

#ifdef __APPLE__
void* VulkanWindow::GetMetalLayer() const
{
	return m_metalLayer;
}
#endif
