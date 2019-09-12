#pragma once

#include "outputwindow.h"

class VulkanWindow : public OutputWindow
{
	Q_OBJECT
public:
	explicit VulkanWindow(QWindow* parent = 0);
	~VulkanWindow() = default;
};
