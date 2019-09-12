#include "vulkanwindow.h"

VulkanWindow::VulkanWindow(QWindow* parent)
    : OutputWindow(parent)
{
	setSurfaceType(QWindow::VulkanSurface);
}
