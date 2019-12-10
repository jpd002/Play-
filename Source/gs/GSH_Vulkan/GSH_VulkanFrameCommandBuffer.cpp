#include "GSH_VulkanFrameCommandBuffer.h"
#include "vulkan/StructDefs.h"

using namespace GSH_Vulkan;

CFrameCommandBuffer::CFrameCommandBuffer(const ContextPtr& context)
	: m_context(context)
{

}

void CFrameCommandBuffer::RegisterWriter(IFrameCommandBufferWriter* writer)
{
	m_writers.push_back(writer);
}

VkCommandBuffer CFrameCommandBuffer::GetCommandBuffer()
{
	if(m_commandBuffer != VK_NULL_HANDLE) return m_commandBuffer;

	auto result = VK_SUCCESS;
	m_commandBuffer = m_context->commandBufferPool.AllocateBuffer();

	auto commandBufferBeginInfo = Framework::Vulkan::CommandBufferBeginInfo();
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	result = m_context->device.vkBeginCommandBuffer(m_commandBuffer, &commandBufferBeginInfo);
	CHECKVULKANERROR(result);

	return m_commandBuffer;
}

void CFrameCommandBuffer::Flush()
{
	for(const auto& writer : m_writers)
	{
		writer->PreFlushFrameCommandBuffer();
	}

	if(m_commandBuffer != VK_NULL_HANDLE)
	{
		m_context->device.vkEndCommandBuffer(m_commandBuffer);

		auto result = VK_SUCCESS;

		//Submit command buffer
		{
			auto submitInfo = Framework::Vulkan::SubmitInfo();
			submitInfo.commandBufferCount   = 1;
			submitInfo.pCommandBuffers      = &m_commandBuffer;
			result = m_context->device.vkQueueSubmit(m_context->queue, 1, &submitInfo, VK_NULL_HANDLE);
			CHECKVULKANERROR(result);
		}

		result = m_context->device.vkQueueWaitIdle(m_context->queue);
		CHECKVULKANERROR(result);

		m_commandBuffer = VK_NULL_HANDLE;
	}

	for(const auto& writer : m_writers)
	{
		writer->PostFlushFrameCommandBuffer();
	}
}
