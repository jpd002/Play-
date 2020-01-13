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

		void BeginFrame();
		uint32 GetSubmitCount() const;

		VkCommandBuffer GetCommandBuffer();
		void Flush();

	private:
		ContextPtr m_context;

		std::vector<IFrameCommandBufferWriter*> m_writers;
		VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;

		uint32 m_submitCount = 0;
	};

	typedef std::shared_ptr<CFrameCommandBuffer> FrameCommandBufferPtr;
}
