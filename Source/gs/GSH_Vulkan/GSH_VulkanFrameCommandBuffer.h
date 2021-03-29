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
		enum
		{
			MAX_FRAMES = 3,
		};

		CFrameCommandBuffer(const ContextPtr&);
		~CFrameCommandBuffer();

		void RegisterWriter(IFrameCommandBufferWriter*);

		void BeginFrame();
		void EndFrame();
		void Flush();

		uint32 GetFlushCount() const;
		void ResetFlushCount();

		VkCommandBuffer GetCommandBuffer();
		uint32 GetCurrentFrame() const;

	private:
		struct FRAMECONTEXT
		{
			VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
			VkFence execCompleteFence = VK_NULL_HANDLE;
		};

		ContextPtr m_context;

		std::vector<IFrameCommandBufferWriter*> m_writers;

		FRAMECONTEXT m_frames[MAX_FRAMES];
		uint32 m_currentFrame = 0;

		uint32 m_flushCount = 0;
	};

	typedef std::shared_ptr<CFrameCommandBuffer> FrameCommandBufferPtr;
}
