#include "vulkanwindow.h"
#include "macos/LayerFromView.h"

VulkanWindow::VulkanWindow(QWindow* parent)
    : OutputWindow(parent)
{
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
