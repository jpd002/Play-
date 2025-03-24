#pragma once

#include "outputwindow.h"

class VulkanWindow : public OutputWindow
{
	Q_OBJECT
public:
	explicit VulkanWindow(QWindow* parent = 0);
	~VulkanWindow() = default;

#ifdef __APPLE__
	void* GetMetalLayer() const;
#endif

private:
#ifdef __APPLE__
	void* m_metalLayer = nullptr;
#endif
};
