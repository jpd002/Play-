#include "GSH_VulkanFrameCommandBuffer.h"
#include "vulkan/StructDefs.h"

using namespace GSH_Vulkan;

CFrameCommandBuffer::CFrameCommandBuffer(const ContextPtr& context)
    : m_context(context)
{
	auto result = VK_SUCCESS;

	for(auto& frame : m_frames)
	{
		frame.commandBuffer = m_context->commandBufferPool.AllocateBuffer();

		{
			auto fenceCreateInfo = Framework::Vulkan::FenceCreateInfo();
			fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			result = m_context->device.vkCreateFence(m_context->device, &fenceCreateInfo, nullptr, &frame.execCompleteFence);
			CHECKVULKANERROR(result);
		}
	}
}

CFrameCommandBuffer::~CFrameCommandBuffer()
{
	for(auto& frame : m_frames)
	{
		m_context->device.vkDestroyFence(m_context->device, frame.execCompleteFence, nullptr);
	}
}

void CFrameCommandBuffer::RegisterWriter(IFrameCommandBufferWriter* writer)
{
	m_writers.push_back(writer);
}

void CFrameCommandBuffer::BeginFrame()
{
	const auto& frame = m_frames[m_currentFrame];

	auto result = VK_SUCCESS;

	result = m_context->device.vkWaitForFences(m_context->device, 1, &frame.execCompleteFence, VK_TRUE, UINT64_MAX);
	CHECKVULKANERROR(result);

	result = m_context->device.vkResetFences(m_context->device, 1, &frame.execCompleteFence);
	CHECKVULKANERROR(result);

	result = m_context->device.vkResetCommandBuffer(frame.commandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
	CHECKVULKANERROR(result);

	auto commandBufferBeginInfo = Framework::Vulkan::CommandBufferBeginInfo();
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	result = m_context->device.vkBeginCommandBuffer(frame.commandBuffer, &commandBufferBeginInfo);
	CHECKVULKANERROR(result);
}

void CFrameCommandBuffer::EndFrame()
{
	for(const auto& writer : m_writers)
	{
		writer->PreFlushFrameCommandBuffer();
	}

	auto result = VK_SUCCESS;
	const auto& frame = m_frames[m_currentFrame];

	result = m_context->device.vkEndCommandBuffer(frame.commandBuffer);
	CHECKVULKANERROR(result);

	//Submit command buffer
	{
		auto submitInfo = Framework::Vulkan::SubmitInfo();
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &frame.commandBuffer;
		result = m_context->device.vkQueueSubmit(m_context->queue, 1, &submitInfo, frame.execCompleteFence);
		CHECKVULKANERROR(result);
	}

	for(const auto& writer : m_writers)
	{
		writer->PostFlushFrameCommandBuffer();
	}

	m_currentFrame++;
	m_currentFrame %= MAX_FRAMES;
}

void CFrameCommandBuffer::Flush()
{
	EndFrame();
	BeginFrame();
	m_flushCount++;
}

uint32 CFrameCommandBuffer::GetFlushCount() const
{
	return m_flushCount;
}

void CFrameCommandBuffer::ResetFlushCount()
{
	m_flushCount = 0;
}

VkCommandBuffer CFrameCommandBuffer::GetCommandBuffer()
{
	const auto& frame = m_frames[m_currentFrame];
	return frame.commandBuffer;
}

uint32 CFrameCommandBuffer::GetCurrentFrame() const
{
	return m_currentFrame;
}
