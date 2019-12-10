#pragma once

#include "GSH_VulkanContext.h"

namespace GSH_Vulkan
{
	class IFrameCommandBufferWriter
	{
	public:
		virtual ~IFrameCommandBufferWriter() = default;
		virtual void PreFlushFrameCommandBuffer() = 0;
		virtual void PostFlushFrameCommandBuffer() = 0;
	};

	class CFrameCommandBuffer
	{
	public:
		CFrameCommandBuffer(const ContextPtr&);

		void RegisterWriter(IFrameCommandBufferWriter*);

		VkCommandBuffer GetCommandBuffer();
		void Flush();

	private:
		ContextPtr m_context;

		std::vector<IFrameCommandBufferWriter*> m_writers;
		VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
	};

	typedef std::shared_ptr<CFrameCommandBuffer> FrameCommandBufferPtr;
}
