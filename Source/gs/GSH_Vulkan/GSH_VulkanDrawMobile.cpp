#include "GSH_VulkanDrawMobile.h"
#include "GSH_VulkanDrawUtils.h"
#include "GSH_VulkanMemoryUtils.h"
#include "MemStream.h"
#include "vulkan/StructDefs.h"
#include "vulkan/Utils.h"
#include "nuanceur/Builder.h"
#include "nuanceur/generators/SpirvShaderGenerator.h"
#include "../GSHandler.h"
#include "../GsPixelFormats.h"

using namespace GSH_Vulkan;

#define VERTEX_ATTRIB_LOCATION_POSITION 0
#define VERTEX_ATTRIB_LOCATION_DEPTH 1
#define VERTEX_ATTRIB_LOCATION_COLOR 2
#define VERTEX_ATTRIB_LOCATION_TEXCOORD 3
#define VERTEX_ATTRIB_LOCATION_FOG 4

#define DESCRIPTOR_LOCATION_BUFFER_MEMORY 0
#define DESCRIPTOR_LOCATION_IMAGE_CLUT 1
#define DESCRIPTOR_LOCATION_IMAGE_SWIZZLETABLE_TEX 2
#define DESCRIPTOR_LOCATION_IMAGE_SWIZZLETABLE_FB 3
#define DESCRIPTOR_LOCATION_IMAGE_SWIZZLETABLE_DEPTH 4
#define DESCRIPTOR_LOCATION_IMAGE_INPUT_COLOR 5
#define DESCRIPTOR_LOCATION_IMAGE_INPUT_DEPTH 6

#define DRAW_AREA_SIZE 2048
#define MAX_VERTEX_COUNT 1024 * 512

#define DEPTH_MAX (4294967296.0f)

CDrawMobile::CDrawMobile(const ContextPtr& context, const FrameCommandBufferPtr& frameCommandBuffer)
    : CDraw(context, frameCommandBuffer)
    , m_loadPipelineCache(context->device)
    , m_storePipelineCache(context->device)
{
	CreateRenderPass();
	CreateDrawImages();
	CreateFramebuffer();
}

CDrawMobile::~CDrawMobile()
{
	m_context->device.vkDestroyFramebuffer(m_context->device, m_framebuffer, nullptr);
	m_context->device.vkDestroyRenderPass(m_context->device, m_renderPass, nullptr);
	m_context->device.vkDestroyImageView(m_context->device, m_drawColorImageView, nullptr);
	m_context->device.vkDestroyImageView(m_context->device, m_drawDepthImageView, nullptr);
}

void CDrawMobile::SetPipelineCaps(const PIPELINE_CAPS& caps)
{
	bool changed = static_cast<uint64>(caps) != static_cast<uint64>(m_pipelineCaps);
	if(!changed) return;
	auto prevLoadStoreCaps = MakeLoadStorePipelineCaps(m_pipelineCaps);
	auto nextLoadStoreCaps = MakeLoadStorePipelineCaps(caps);
	if(static_cast<uint64>(prevLoadStoreCaps) != static_cast<uint64>(nextLoadStoreCaps))
	{
		FlushRenderPass();
	}
	if(caps.textureUseMemoryCopy)
	{
		FlushRenderPass();
	}
	FlushVertices();
	m_pipelineCaps = caps;
}

void CDrawMobile::SetFramebufferParams(uint32 addr, uint32 width, uint32 writeMask)
{
	bool storeChanged =
	    (m_pushConstants.fbBufAddr != addr) ||
	    (m_pushConstants.fbBufWidth != width);
	bool drawChanged =
	    (m_pushConstants.fbWriteMask != writeMask);
	if(!storeChanged && !drawChanged) return;
	if(storeChanged)
	{
		FlushRenderPass();
	}
	if(drawChanged)
	{
		FlushVertices();
	}
	m_pushConstants.fbBufAddr = addr;
	m_pushConstants.fbBufWidth = width;
	m_pushConstants.fbWriteMask = writeMask;
}

void CDrawMobile::SetDepthbufferParams(uint32 addr, uint32 width)
{
	bool changed =
	    (m_pushConstants.depthBufAddr != addr) ||
	    (m_pushConstants.depthBufWidth != width);
	if(!changed) return;
	FlushRenderPass();
	m_pushConstants.depthBufAddr = addr;
	m_pushConstants.depthBufWidth = width;
}

void CDrawMobile::SetScissor(uint32 scissorX, uint32 scissorY, uint32 scissorWidth, uint32 scissorHeight)
{
	bool changed =
	    (m_scissorX != scissorX) ||
	    (m_scissorY != scissorY) ||
	    (m_scissorWidth != scissorWidth) ||
	    (m_scissorHeight != scissorHeight);
	if(!changed) return;
	FlushRenderPass();
	m_scissorX = scissorX;
	m_scissorY = scissorY;
	m_scissorWidth = scissorWidth;
	m_scissorHeight = scissorHeight;
}

void CDrawMobile::FlushVertices()
{
	uint32 vertexCount = m_passVertexEnd - m_passVertexStart;
	if(vertexCount == 0) return;

	auto& frame = m_frames[m_frameCommandBuffer->GetCurrentFrame()];
	auto commandBuffer = m_frameCommandBuffer->GetCommandBuffer();

	for(auto vertex = frame.vertexBufferPtr + m_passVertexStart; vertex != frame.vertexBufferPtr + m_passVertexEnd; vertex++)
	{
		m_renderPassMinX = std::min(vertex->x, m_renderPassMinX);
		m_renderPassMinY = std::min(vertex->y, m_renderPassMinY);
		m_renderPassMaxX = std::max(vertex->x, m_renderPassMaxX);
		m_renderPassMaxY = std::max(vertex->y, m_renderPassMaxY);
	}

	if(m_pipelineCaps.textureUseMemoryCopy)
	{
		assert(!m_renderPassBegun);
		m_memoryCopyRegion.Reset();
	}

	{
		VkViewport viewport = {};
		viewport.width = DRAW_AREA_SIZE;
		viewport.height = DRAW_AREA_SIZE;
		viewport.maxDepth = 1.0f;
		m_context->device.vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset.x = m_scissorX;
		scissor.offset.y = m_scissorY;
		scissor.extent.width = m_scissorWidth;
		scissor.extent.height = m_scissorHeight;
		m_context->device.vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	if(!m_renderPassBegun)
	{
		auto renderPassBeginInfo = Framework::Vulkan::RenderPassBeginInfo();
		renderPassBeginInfo.renderPass = m_renderPass;
		renderPassBeginInfo.renderArea.extent.width = DRAW_AREA_SIZE;
		renderPassBeginInfo.renderArea.extent.height = DRAW_AREA_SIZE;
		renderPassBeginInfo.framebuffer = m_framebuffer;
		m_context->device.vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		//Load from memory

		//Find pipeline and create it if we've never encountered it before
		auto strippedCaps = MakeLoadStorePipelineCaps(m_pipelineCaps);
		auto loadPipeline = m_loadPipelineCache.TryGetPipeline(strippedCaps);
		if(!loadPipeline)
		{
			loadPipeline = m_loadPipelineCache.RegisterPipeline(strippedCaps, CreateLoadPipeline(strippedCaps));
		}

		auto descriptorSetCaps = make_convertible<DESCRIPTORSET_CAPS>(0);
		descriptorSetCaps.framebufferFormat = m_pipelineCaps.framebufferFormat;
		descriptorSetCaps.depthbufferFormat = m_pipelineCaps.depthbufferFormat;

		auto descriptorSet = PrepareDescriptorSet(loadPipeline->descriptorSetLayout, descriptorSetCaps);

		m_context->device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, loadPipeline->pipelineLayout,
		                                          0, 1, &descriptorSet, 0, nullptr);
		m_context->device.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, loadPipeline->pipeline);
		m_context->device.vkCmdPushConstants(commandBuffer, loadPipeline->pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
		                                     0, sizeof(DRAW_PIPELINE_PUSHCONSTANTS), &m_pushConstants);
		m_context->device.vkCmdDraw(commandBuffer, 3, 1, 0, 0);

		m_context->device.vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);

		m_renderPassBegun = true;
	}

	//Find pipeline and create it if we've never encountered it before
	auto drawPipeline = m_pipelineCache.TryGetPipeline(m_pipelineCaps);
	if(!drawPipeline)
	{
		drawPipeline = m_pipelineCache.RegisterPipeline(m_pipelineCaps, CreateDrawPipeline(m_pipelineCaps));
	}

	{
		auto memoryBarrier = Framework::Vulkan::MemoryBarrier();
		memoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		memoryBarrier.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;

		m_context->device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		                                       VK_DEPENDENCY_BY_REGION_BIT, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
	}

	auto descriptorSetCaps = make_convertible<DESCRIPTORSET_CAPS>(0);
	descriptorSetCaps.hasTexture = m_pipelineCaps.hasTexture;
	descriptorSetCaps.framebufferFormat = m_pipelineCaps.framebufferFormat;
	descriptorSetCaps.depthbufferFormat = m_pipelineCaps.depthbufferFormat;
	descriptorSetCaps.textureFormat = m_pipelineCaps.textureFormat;

	auto descriptorSet = PrepareDescriptorSet(drawPipeline->descriptorSetLayout, descriptorSetCaps);

	std::vector<uint32> descriptorDynamicOffsets;

	if(m_pipelineCaps.hasTexture && CGsPixelFormats::IsPsmIDTEX(m_pipelineCaps.textureFormat))
	{
		descriptorDynamicOffsets.push_back(m_clutBufferOffset);
	}

	m_context->device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, drawPipeline->pipelineLayout,
	                                          0, 1, &descriptorSet, static_cast<uint32_t>(descriptorDynamicOffsets.size()), descriptorDynamicOffsets.data());

	m_context->device.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, drawPipeline->pipeline);

	VkDeviceSize vertexBufferOffset = (m_passVertexStart * sizeof(PRIM_VERTEX));
	VkBuffer vertexBuffer = frame.vertexBuffer;
	m_context->device.vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &vertexBufferOffset);

	m_context->device.vkCmdPushConstants(commandBuffer, drawPipeline->pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
	                                     0, sizeof(DRAW_PIPELINE_PUSHCONSTANTS), &m_pushConstants);

	m_context->device.vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);

	m_passVertexStart = m_passVertexEnd;
}

void CDrawMobile::FlushRenderPass()
{
	FlushVertices();
	if(m_renderPassBegun)
	{
		auto commandBuffer = m_frameCommandBuffer->GetCommandBuffer();
		m_context->device.vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);

		//Store to memory

		int32 clippedX0 = std::clamp<int32>(m_renderPassMinX, m_scissorX, m_scissorX + m_scissorWidth);
		int32 clippedX1 = std::clamp<int32>(m_renderPassMaxX, m_scissorX, m_scissorX + m_scissorWidth);
		int32 clippedY0 = std::clamp<int32>(m_renderPassMinY, m_scissorY, m_scissorY + m_scissorHeight);
		int32 clippedY1 = std::clamp<int32>(m_renderPassMaxY, m_scissorY, m_scissorY + m_scissorHeight);

		VkRect2D scissor = {};
		scissor.offset.x = clippedX0;
		scissor.offset.y = clippedY0;
		scissor.extent.width = clippedX1 - clippedX0;
		scissor.extent.height = clippedY1 - clippedY0;
		m_context->device.vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		//Find pipeline and create it if we've never encountered it before
		auto strippedCaps = MakeLoadStorePipelineCaps(m_pipelineCaps);
		auto storePipeline = m_storePipelineCache.TryGetPipeline(strippedCaps);
		if(!storePipeline)
		{
			storePipeline = m_storePipelineCache.RegisterPipeline(strippedCaps, CreateStorePipeline(strippedCaps));
		}

		auto descriptorSetCaps = make_convertible<DESCRIPTORSET_CAPS>(0);
		descriptorSetCaps.framebufferFormat = m_pipelineCaps.framebufferFormat;
		descriptorSetCaps.depthbufferFormat = m_pipelineCaps.depthbufferFormat;

		auto descriptorSet = PrepareDescriptorSet(storePipeline->descriptorSetLayout, descriptorSetCaps);

		m_context->device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, storePipeline->pipelineLayout,
		                                          0, 1, &descriptorSet, 0, nullptr);
		m_context->device.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, storePipeline->pipeline);
		m_context->device.vkCmdPushConstants(commandBuffer, storePipeline->pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
		                                     0, sizeof(DRAW_PIPELINE_PUSHCONSTANTS), &m_pushConstants);
		m_context->device.vkCmdDraw(commandBuffer, 3, 1, 0, 0);

		m_context->device.vkCmdEndRenderPass(commandBuffer);
		m_renderPassBegun = false;
	}
	m_renderPassMinX = FLT_MAX;
	m_renderPassMinY = FLT_MAX;
	m_renderPassMaxX = -FLT_MAX;
	m_renderPassMaxY = -FLT_MAX;
}

VkDescriptorSet CDrawMobile::PrepareDescriptorSet(VkDescriptorSetLayout descriptorSetLayout, const DESCRIPTORSET_CAPS& caps)
{
	auto descriptorSetIterator = m_descriptorSetCache.find(caps);
	if(descriptorSetIterator != std::end(m_descriptorSetCache))
	{
		return descriptorSetIterator->second;
	}

	auto result = VK_SUCCESS;
	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

	//Allocate descriptor set
	{
		auto setAllocateInfo = Framework::Vulkan::DescriptorSetAllocateInfo();
		setAllocateInfo.descriptorPool = m_context->descriptorPool;
		setAllocateInfo.descriptorSetCount = 1;
		setAllocateInfo.pSetLayouts = &descriptorSetLayout;

		result = m_context->device.vkAllocateDescriptorSets(m_context->device, &setAllocateInfo, &descriptorSet);
		CHECKVULKANERROR(result);
	}

	//Update descriptor set
	{
		VkDescriptorBufferInfo descriptorMemoryBufferInfo = {};
		descriptorMemoryBufferInfo.buffer = m_context->memoryBuffer;
		descriptorMemoryBufferInfo.range = VK_WHOLE_SIZE;

		VkDescriptorBufferInfo descriptorClutBufferInfo = {};
		descriptorClutBufferInfo.buffer = m_context->clutBuffer;
		descriptorClutBufferInfo.range = sizeof(uint32) * CGSHandler::CLUTENTRYCOUNT;

		VkDescriptorImageInfo descriptorTexSwizzleTableImageInfo = {};
		descriptorTexSwizzleTableImageInfo.imageView = m_context->GetSwizzleTable(caps.textureFormat);
		descriptorTexSwizzleTableImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkDescriptorImageInfo descriptorFbSwizzleTableImageInfo = {};
		descriptorFbSwizzleTableImageInfo.imageView = m_context->GetSwizzleTable(caps.framebufferFormat);
		descriptorFbSwizzleTableImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkDescriptorImageInfo descriptorDepthSwizzleTableImageInfo = {};
		descriptorDepthSwizzleTableImageInfo.imageView = m_context->GetSwizzleTable(caps.depthbufferFormat);
		descriptorDepthSwizzleTableImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkDescriptorImageInfo descriptorInputColorImageInfo = {};
		descriptorInputColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		descriptorInputColorImageInfo.imageView = m_drawColorImageView;

		VkDescriptorImageInfo descriptorInputDepthImageInfo = {};
		descriptorInputDepthImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		descriptorInputDepthImageInfo.imageView = m_drawDepthImageView;

		std::vector<VkWriteDescriptorSet> writes;

		{
			auto writeSet = Framework::Vulkan::WriteDescriptorSet();
			writeSet.dstSet = descriptorSet;
			writeSet.dstBinding = DESCRIPTOR_LOCATION_BUFFER_MEMORY;
			writeSet.descriptorCount = 1;
			writeSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			writeSet.pBufferInfo = &descriptorMemoryBufferInfo;
			writes.push_back(writeSet);
		}

		{
			auto writeSet = Framework::Vulkan::WriteDescriptorSet();
			writeSet.dstSet = descriptorSet;
			writeSet.dstBinding = DESCRIPTOR_LOCATION_IMAGE_SWIZZLETABLE_FB;
			writeSet.descriptorCount = 1;
			writeSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			writeSet.pImageInfo = &descriptorFbSwizzleTableImageInfo;
			writes.push_back(writeSet);
		}

		{
			auto writeSet = Framework::Vulkan::WriteDescriptorSet();
			writeSet.dstSet = descriptorSet;
			writeSet.dstBinding = DESCRIPTOR_LOCATION_IMAGE_SWIZZLETABLE_DEPTH;
			writeSet.descriptorCount = 1;
			writeSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			writeSet.pImageInfo = &descriptorDepthSwizzleTableImageInfo;
			writes.push_back(writeSet);
		}

		if(caps.hasTexture)
		{
			{
				auto writeSet = Framework::Vulkan::WriteDescriptorSet();
				writeSet.dstSet = descriptorSet;
				writeSet.dstBinding = DESCRIPTOR_LOCATION_IMAGE_SWIZZLETABLE_TEX;
				writeSet.descriptorCount = 1;
				writeSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				writeSet.pImageInfo = &descriptorTexSwizzleTableImageInfo;
				writes.push_back(writeSet);
			}

			if(CGsPixelFormats::IsPsmIDTEX(caps.textureFormat))
			{
				auto writeSet = Framework::Vulkan::WriteDescriptorSet();
				writeSet.dstSet = descriptorSet;
				writeSet.dstBinding = DESCRIPTOR_LOCATION_IMAGE_CLUT;
				writeSet.descriptorCount = 1;
				writeSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
				writeSet.pBufferInfo = &descriptorClutBufferInfo;
				writes.push_back(writeSet);
			}
		}

		{
			auto writeSet = Framework::Vulkan::WriteDescriptorSet();
			writeSet.dstSet = descriptorSet;
			writeSet.dstBinding = DESCRIPTOR_LOCATION_IMAGE_INPUT_COLOR;
			writeSet.descriptorCount = 1;
			writeSet.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			writeSet.pImageInfo = &descriptorInputColorImageInfo;
			writes.push_back(writeSet);
		}

		{
			auto writeSet = Framework::Vulkan::WriteDescriptorSet();
			writeSet.dstSet = descriptorSet;
			writeSet.dstBinding = DESCRIPTOR_LOCATION_IMAGE_INPUT_DEPTH;
			writeSet.descriptorCount = 1;
			writeSet.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			writeSet.pImageInfo = &descriptorInputDepthImageInfo;
			writes.push_back(writeSet);
		}

		m_context->device.vkUpdateDescriptorSets(m_context->device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
	}

	m_descriptorSetCache.insert(std::make_pair(caps, descriptorSet));

	return descriptorSet;
}

void CDrawMobile::CreateFramebuffer()
{
	assert(m_renderPass != VK_NULL_HANDLE);
	assert(m_framebuffer == VK_NULL_HANDLE);

	std::vector<VkImageView> attachments;
	attachments.push_back(m_drawColorImageView);
	attachments.push_back(m_drawDepthImageView);

	auto frameBufferCreateInfo = Framework::Vulkan::FramebufferCreateInfo();
	frameBufferCreateInfo.renderPass = m_renderPass;
	frameBufferCreateInfo.width = DRAW_AREA_SIZE;
	frameBufferCreateInfo.height = DRAW_AREA_SIZE;
	frameBufferCreateInfo.layers = 1;
	frameBufferCreateInfo.attachmentCount = attachments.size();
	frameBufferCreateInfo.pAttachments = attachments.data();

	auto result = m_context->device.vkCreateFramebuffer(m_context->device, &frameBufferCreateInfo, nullptr, &m_framebuffer);
	CHECKVULKANERROR(result);
}

void CDrawMobile::CreateRenderPass()
{
	assert(m_renderPass == VK_NULL_HANDLE);

	auto result = VK_SUCCESS;

	VkAttachmentReference colorRef = {};
	colorRef.attachment = 0;
	colorRef.layout = VK_IMAGE_LAYOUT_GENERAL;

	VkAttachmentReference depthRef = {};
	depthRef.attachment = 1;
	depthRef.layout = VK_IMAGE_LAYOUT_GENERAL;

	std::vector<VkAttachmentReference> attachmentRefs;
	attachmentRefs.push_back(colorRef);
	attachmentRefs.push_back(depthRef);

	std::vector<VkSubpassDescription> subpasses;

	//Unswizzle Subpass
	{
		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.pColorAttachments = attachmentRefs.data();
		subpass.colorAttachmentCount = attachmentRefs.size();
		subpasses.push_back(subpass);
	}

	//Draw Subpass
	{
		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.pColorAttachments = attachmentRefs.data();
		subpass.colorAttachmentCount = attachmentRefs.size();
		subpass.pInputAttachments = attachmentRefs.data();
		subpass.inputAttachmentCount = attachmentRefs.size();
		subpasses.push_back(subpass);
	}

	//Swizzle pass
	{
		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.pColorAttachments = attachmentRefs.data();
		subpass.colorAttachmentCount = attachmentRefs.size();
		subpass.pInputAttachments = attachmentRefs.data();
		subpass.inputAttachmentCount = attachmentRefs.size();
		subpasses.push_back(subpass);
	}

	std::vector<VkSubpassDependency> subpassDependencies;

	{
		VkSubpassDependency subpassDependency = {};
		subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		subpassDependency.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		subpassDependency.dstSubpass = 0;
		subpassDependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		subpassDependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		subpassDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		subpassDependencies.push_back(subpassDependency);
	}

	{
		VkSubpassDependency subpassDependency = {};
		subpassDependency.srcSubpass = 0;
		subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependency.dstSubpass = 1;
		subpassDependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		subpassDependency.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
		subpassDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		subpassDependencies.push_back(subpassDependency);
	}

	{
		VkSubpassDependency subpassDependency = {};
		subpassDependency.srcSubpass = 1;
		subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependency.dstSubpass = 1;
		subpassDependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		subpassDependency.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
		subpassDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		subpassDependencies.push_back(subpassDependency);
	}

	{
		VkSubpassDependency subpassDependency = {};
		subpassDependency.srcSubpass = 1;
		subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependency.dstSubpass = 2;
		subpassDependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		subpassDependency.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
		subpassDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		subpassDependencies.push_back(subpassDependency);
	}

	{
		VkSubpassDependency subpassDependency = {};
		subpassDependency.srcSubpass = 2;
		subpassDependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		subpassDependency.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		subpassDependency.dstSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		subpassDependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		subpassDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		subpassDependencies.push_back(subpassDependency);
	}

	std::vector<VkAttachmentDescription> attachments;

	{
		VkAttachmentDescription attachment = {};
		attachment.format = VK_FORMAT_R32_UINT;
		attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachments.push_back(attachment);
	}

	{
		VkAttachmentDescription attachment = {};
		attachment.format = VK_FORMAT_R32_UINT;
		attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachments.push_back(attachment);
	}

	auto renderPassCreateInfo = Framework::Vulkan::RenderPassCreateInfo();
	renderPassCreateInfo.subpassCount = subpasses.size();
	renderPassCreateInfo.pSubpasses = subpasses.data();
	renderPassCreateInfo.attachmentCount = attachments.size();
	renderPassCreateInfo.pAttachments = attachments.data();
	renderPassCreateInfo.dependencyCount = subpassDependencies.size();
	renderPassCreateInfo.pDependencies = subpassDependencies.data();

	result = m_context->device.vkCreateRenderPass(m_context->device, &renderPassCreateInfo, nullptr, &m_renderPass);
	CHECKVULKANERROR(result);
}

PIPELINE CDrawMobile::CreateDrawPipeline(const PIPELINE_CAPS& caps)
{
	PIPELINE drawPipeline;

	auto vertexShader = CreateVertexShader(caps);
	auto fragmentShader = CreateDrawFragmentShader(caps);

	auto result = VK_SUCCESS;

	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;

		{
			VkDescriptorSetLayoutBinding setLayoutBinding = {};
			setLayoutBinding.binding = DESCRIPTOR_LOCATION_BUFFER_MEMORY;
			setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			setLayoutBinding.descriptorCount = 1;
			setLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			setLayoutBindings.push_back(setLayoutBinding);
		}

		{
			VkDescriptorSetLayoutBinding setLayoutBinding = {};
			setLayoutBinding.binding = DESCRIPTOR_LOCATION_IMAGE_SWIZZLETABLE_FB;
			setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			setLayoutBinding.descriptorCount = 1;
			setLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			setLayoutBindings.push_back(setLayoutBinding);
		}

		{
			VkDescriptorSetLayoutBinding setLayoutBinding = {};
			setLayoutBinding.binding = DESCRIPTOR_LOCATION_IMAGE_SWIZZLETABLE_DEPTH;
			setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			setLayoutBinding.descriptorCount = 1;
			setLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			setLayoutBindings.push_back(setLayoutBinding);
		}

		if(caps.hasTexture)
		{
			{
				VkDescriptorSetLayoutBinding setLayoutBinding = {};
				setLayoutBinding.binding = DESCRIPTOR_LOCATION_IMAGE_SWIZZLETABLE_TEX;
				setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				setLayoutBinding.descriptorCount = 1;
				setLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
				setLayoutBindings.push_back(setLayoutBinding);
			}

			if(CGsPixelFormats::IsPsmIDTEX(caps.textureFormat))
			{
				VkDescriptorSetLayoutBinding setLayoutBinding = {};
				setLayoutBinding.binding = DESCRIPTOR_LOCATION_IMAGE_CLUT;
				setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
				setLayoutBinding.descriptorCount = 1;
				setLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
				setLayoutBindings.push_back(setLayoutBinding);
			}
		}

		{
			VkDescriptorSetLayoutBinding setLayoutBinding = {};
			setLayoutBinding.binding = DESCRIPTOR_LOCATION_IMAGE_INPUT_COLOR;
			setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			setLayoutBinding.descriptorCount = 1;
			setLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			setLayoutBindings.push_back(setLayoutBinding);
		}

		{
			VkDescriptorSetLayoutBinding setLayoutBinding = {};
			setLayoutBinding.binding = DESCRIPTOR_LOCATION_IMAGE_INPUT_DEPTH;
			setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			setLayoutBinding.descriptorCount = 1;
			setLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			setLayoutBindings.push_back(setLayoutBinding);
		}

		auto setLayoutCreateInfo = Framework::Vulkan::DescriptorSetLayoutCreateInfo();
		setLayoutCreateInfo.bindingCount = static_cast<uint32>(setLayoutBindings.size());
		setLayoutCreateInfo.pBindings = setLayoutBindings.data();

		result = m_context->device.vkCreateDescriptorSetLayout(m_context->device, &setLayoutCreateInfo, nullptr, &drawPipeline.descriptorSetLayout);
		CHECKVULKANERROR(result);
	}

	{
		VkPushConstantRange pushConstantInfo = {};
		pushConstantInfo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantInfo.offset = 0;
		pushConstantInfo.size = sizeof(DRAW_PIPELINE_PUSHCONSTANTS);

		auto pipelineLayoutCreateInfo = Framework::Vulkan::PipelineLayoutCreateInfo();
		pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pSetLayouts = &drawPipeline.descriptorSetLayout;

		result = m_context->device.vkCreatePipelineLayout(m_context->device, &pipelineLayoutCreateInfo, nullptr, &drawPipeline.pipelineLayout);
		CHECKVULKANERROR(result);
	}

	auto inputAssemblyInfo = Framework::Vulkan::PipelineInputAssemblyStateCreateInfo();
	switch(caps.primitiveType)
	{
	default:
		assert(false);
	case PIPELINE_PRIMITIVE_TRIANGLE:
		inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		break;
	case PIPELINE_PRIMITIVE_LINE:
		inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		break;
	}

	std::vector<VkVertexInputAttributeDescription> vertexAttributes;

	{
		VkVertexInputAttributeDescription vertexAttributeDesc = {};
		vertexAttributeDesc.format = VK_FORMAT_R32G32_SFLOAT;
		vertexAttributeDesc.offset = offsetof(PRIM_VERTEX, x);
		vertexAttributeDesc.location = VERTEX_ATTRIB_LOCATION_POSITION;
		vertexAttributes.push_back(vertexAttributeDesc);
	}

	{
		VkVertexInputAttributeDescription vertexAttributeDesc = {};
		vertexAttributeDesc.format = VK_FORMAT_R32_UINT;
		vertexAttributeDesc.offset = offsetof(PRIM_VERTEX, z);
		vertexAttributeDesc.location = VERTEX_ATTRIB_LOCATION_DEPTH;
		vertexAttributes.push_back(vertexAttributeDesc);
	}

	{
		VkVertexInputAttributeDescription vertexAttributeDesc = {};
		vertexAttributeDesc.format = VK_FORMAT_R8G8B8A8_UNORM;
		vertexAttributeDesc.offset = offsetof(PRIM_VERTEX, color);
		vertexAttributeDesc.location = VERTEX_ATTRIB_LOCATION_COLOR;
		vertexAttributes.push_back(vertexAttributeDesc);
	}

	{
		VkVertexInputAttributeDescription vertexAttributeDesc = {};
		vertexAttributeDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
		vertexAttributeDesc.offset = offsetof(PRIM_VERTEX, s);
		vertexAttributeDesc.location = VERTEX_ATTRIB_LOCATION_TEXCOORD;
		vertexAttributes.push_back(vertexAttributeDesc);
	}

	{
		VkVertexInputAttributeDescription vertexAttributeDesc = {};
		vertexAttributeDesc.format = VK_FORMAT_R32_SFLOAT;
		vertexAttributeDesc.offset = offsetof(PRIM_VERTEX, f);
		vertexAttributeDesc.location = VERTEX_ATTRIB_LOCATION_FOG;
		vertexAttributes.push_back(vertexAttributeDesc);
	}

	VkVertexInputBindingDescription binding = {};
	binding.stride = sizeof(PRIM_VERTEX);
	binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	auto vertexInputInfo = Framework::Vulkan::PipelineVertexInputStateCreateInfo();
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &binding;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32>(vertexAttributes.size());
	vertexInputInfo.pVertexAttributeDescriptions = vertexAttributes.data();

	auto rasterStateInfo = Framework::Vulkan::PipelineRasterizationStateCreateInfo();
	rasterStateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterStateInfo.cullMode = VK_CULL_MODE_NONE;
	rasterStateInfo.lineWidth = 1.0f;

	// Our attachment will write to all color channels, but no blending is enabled.
	std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates;
	for(uint32 i = 0; i < 2; i++)
	{
		VkPipelineColorBlendAttachmentState blendAttachmentState = {};
		blendAttachmentState.colorWriteMask = 0xf;
		blendAttachmentStates.push_back(blendAttachmentState);
	}

	auto colorBlendStateInfo = Framework::Vulkan::PipelineColorBlendStateCreateInfo();
	colorBlendStateInfo.attachmentCount = blendAttachmentStates.size();
	colorBlendStateInfo.pAttachments = blendAttachmentStates.data();

	auto viewportStateInfo = Framework::Vulkan::PipelineViewportStateCreateInfo();
	viewportStateInfo.viewportCount = 1;
	viewportStateInfo.scissorCount = 1;

	auto depthStencilStateInfo = Framework::Vulkan::PipelineDepthStencilStateCreateInfo();

	auto multisampleStateInfo = Framework::Vulkan::PipelineMultisampleStateCreateInfo();
	multisampleStateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	static const VkDynamicState dynamicStates[] =
	    {
	        VK_DYNAMIC_STATE_VIEWPORT,
	        VK_DYNAMIC_STATE_SCISSOR,
	    };
	auto dynamicStateInfo = Framework::Vulkan::PipelineDynamicStateCreateInfo();
	dynamicStateInfo.pDynamicStates = dynamicStates;
	dynamicStateInfo.dynamicStateCount = sizeof(dynamicStates) / sizeof(dynamicStates[0]);

	VkPipelineShaderStageCreateInfo shaderStages[2] =
	    {
	        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO},
	        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO},
	    };

	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = vertexShader;
	shaderStages[0].pName = "main";
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = fragmentShader;
	shaderStages[1].pName = "main";

	auto pipelineCreateInfo = Framework::Vulkan::GraphicsPipelineCreateInfo();
	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.subpass = 1;
	pipelineCreateInfo.pStages = shaderStages;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyInfo;
	pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
	pipelineCreateInfo.pRasterizationState = &rasterStateInfo;
	pipelineCreateInfo.pColorBlendState = &colorBlendStateInfo;
	pipelineCreateInfo.pViewportState = &viewportStateInfo;
	pipelineCreateInfo.pDepthStencilState = &depthStencilStateInfo;
	pipelineCreateInfo.pMultisampleState = &multisampleStateInfo;
	pipelineCreateInfo.pDynamicState = &dynamicStateInfo;
	pipelineCreateInfo.renderPass = m_renderPass;
	pipelineCreateInfo.layout = drawPipeline.pipelineLayout;

	result = m_context->device.vkCreateGraphicsPipelines(m_context->device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &drawPipeline.pipeline);
	CHECKVULKANERROR(result);

	return drawPipeline;
}

Framework::Vulkan::CShaderModule CDrawMobile::CreateDrawFragmentShader(const PIPELINE_CAPS& caps)
{
	using namespace Nuanceur;

	auto b = CShaderBuilder();

	{
		//Inputs
		auto inputPosition = CFloat4Lvalue(b.CreateInput(Nuanceur::SEMANTIC_SYSTEM_POSITION));
		auto inputDepth = CFloat4Lvalue(b.CreateInput(Nuanceur::SEMANTIC_TEXCOORD, 1));
		auto inputColor = CFloat4Lvalue(b.CreateInput(Nuanceur::SEMANTIC_TEXCOORD, 2));
		auto inputTexCoord = CFloat4Lvalue(b.CreateInput(Nuanceur::SEMANTIC_TEXCOORD, 3));
		auto inputFog = CFloat4Lvalue(b.CreateInput(Nuanceur::SEMANTIC_TEXCOORD, 4));

		//Outputs
		auto outputColor = CUintLvalue(b.CreateOutputUint(Nuanceur::SEMANTIC_SYSTEM_COLOR, 0));
		auto outputDepth = CUintLvalue(b.CreateOutputUint(Nuanceur::SEMANTIC_SYSTEM_COLOR, 1));

		auto memoryBuffer = CArrayUintValue(b.CreateUniformArrayUint("memoryBuffer", DESCRIPTOR_LOCATION_BUFFER_MEMORY, Nuanceur::SYMBOL_ATTRIBUTE_COHERENT));
		auto clutBuffer = CArrayUintValue(b.CreateUniformArrayUint("clutBuffer", DESCRIPTOR_LOCATION_IMAGE_CLUT));
		auto texSwizzleTable = CImageUint2DValue(b.CreateImage2DUint(DESCRIPTOR_LOCATION_IMAGE_SWIZZLETABLE_TEX));
		auto fbSwizzleTable = CImageUint2DValue(b.CreateImage2DUint(DESCRIPTOR_LOCATION_IMAGE_SWIZZLETABLE_FB));
		auto depthSwizzleTable = CImageUint2DValue(b.CreateImage2DUint(DESCRIPTOR_LOCATION_IMAGE_SWIZZLETABLE_DEPTH));
		auto subpassColorInput = CSubpassInputUintValue(b.CreateSubpassInputUint(DESCRIPTOR_LOCATION_IMAGE_INPUT_COLOR, 0));
		auto subpassDepthInput = CSubpassInputUintValue(b.CreateSubpassInputUint(DESCRIPTOR_LOCATION_IMAGE_INPUT_DEPTH, 1));

		//Push constants
		auto fbDepthParams = CInt4Lvalue(b.CreateUniformInt4("fbDepthParams", Nuanceur::UNIFORM_UNIT_PUSHCONSTANT));
		auto texParams0 = CInt4Lvalue(b.CreateUniformInt4("texParams0", Nuanceur::UNIFORM_UNIT_PUSHCONSTANT));
		auto texParams1 = CInt4Lvalue(b.CreateUniformInt4("texParams1", Nuanceur::UNIFORM_UNIT_PUSHCONSTANT));
		auto texParams2 = CInt4Lvalue(b.CreateUniformInt4("texParams2", Nuanceur::UNIFORM_UNIT_PUSHCONSTANT));
		auto alphaFbParams = CInt4Lvalue(b.CreateUniformInt4("alphaFbParams", Nuanceur::UNIFORM_UNIT_PUSHCONSTANT));
		auto fogColor = CFloat4Lvalue(b.CreateUniformFloat4("fogColor", Nuanceur::UNIFORM_UNIT_PUSHCONSTANT));

		auto fbBufAddress = fbDepthParams->x();
		auto fbBufWidth = fbDepthParams->y();
		auto depthBufAddress = fbDepthParams->z();
		auto depthBufWidth = fbDepthParams->w();

		auto texBufAddress = texParams0->x();
		auto texBufWidth = texParams0->y();
		auto texSize = texParams0->zw();

		auto texCsa = texParams1->y();
		auto texA0 = ToFloat(texParams1->z()) / NewFloat(b, 255.f);
		auto texA1 = ToFloat(texParams1->w()) / NewFloat(b, 255.f);

		auto clampMin = texParams2->xy();
		auto clampMax = texParams2->zw();

		auto fbWriteMask = ToUint(alphaFbParams->x());
		auto alphaFix = alphaFbParams->y();
		auto alphaRef = alphaFbParams->z();

		auto srcDepth = ToUint(inputDepth->x() * NewFloat(b, DEPTH_MAX));

		//TODO: Try vectorized shift
		//auto imageColor = ToUint(inputColor * NewFloat4(b, 255.f, 255.f, 255.f, 255.f));

		auto textureColor = CFloat4Lvalue(b.CreateVariableFloat("textureColor"));
		textureColor = NewFloat4(b, 1, 1, 1, 1);

		if(caps.hasTexture)
		{
			auto clampCoordinates =
			    [&](CInt2Value textureIuv) {
				    auto clampU = CDrawUtils::ClampTexCoord(b, caps.texClampU, textureIuv->x(), texSize->x(), clampMin->x(), clampMax->x());
				    auto clampV = CDrawUtils::ClampTexCoord(b, caps.texClampV, textureIuv->y(), texSize->y(), clampMin->y(), clampMax->y());
				    return NewInt2(clampU, clampV);
			    };

			auto getTextureColor =
			    [&](CInt2Value textureIuv, CFloat4Lvalue& textureColor) {
				    textureColor = CDrawUtils::GetTextureColor(b, caps.textureFormat, caps.clutFormat, textureIuv,
				                                               memoryBuffer, clutBuffer, texSwizzleTable, texBufAddress, texBufWidth, texCsa);
				    if(caps.textureHasAlpha)
				    {
					    CDrawUtils::ExpandAlpha(b, caps.textureFormat, caps.clutFormat, caps.textureBlackIsTransparent, textureColor, texA0, texA1);
				    }
			    };

			auto textureSt = CFloat2Lvalue(b.CreateVariableFloat("textureSt"));
			textureSt = inputTexCoord->xy() / inputTexCoord->zz();

			if(caps.textureUseLinearFiltering)
			{
				//Linear Sampling
				//-------------------------------------

				auto textureLinearPos = CFloat2Lvalue(b.CreateVariableFloat("textureLinearPos"));
				auto textureLinearAb = CFloat2Lvalue(b.CreateVariableFloat("textureLinearAb"));
				auto textureIuv0 = CInt2Lvalue(b.CreateVariableInt("textureIuv0"));
				auto textureIuv1 = CInt2Lvalue(b.CreateVariableInt("textureIuv1"));
				auto textureColorA = CFloat4Lvalue(b.CreateVariableFloat("textureColorA"));
				auto textureColorB = CFloat4Lvalue(b.CreateVariableFloat("textureColorB"));
				auto textureColorC = CFloat4Lvalue(b.CreateVariableFloat("textureColorC"));
				auto textureColorD = CFloat4Lvalue(b.CreateVariableFloat("textureColorD"));

				textureLinearPos = (textureSt * ToFloat(texSize)) + NewFloat2(b, -0.5f, -0.5f);
				textureLinearAb = Fract(textureLinearPos);

				textureIuv0 = ToInt(textureLinearPos);
				textureIuv1 = textureIuv0 + NewInt2(b, 1, 1);

				auto textureClampIuv0 = clampCoordinates(textureIuv0);
				auto textureClampIuv1 = clampCoordinates(textureIuv1);

				getTextureColor(NewInt2(textureClampIuv0->x(), textureClampIuv0->y()), textureColorA);
				getTextureColor(NewInt2(textureClampIuv1->x(), textureClampIuv0->y()), textureColorB);
				getTextureColor(NewInt2(textureClampIuv0->x(), textureClampIuv1->y()), textureColorC);
				getTextureColor(NewInt2(textureClampIuv1->x(), textureClampIuv1->y()), textureColorD);

				auto factorA = (NewFloat(b, 1.0f) - textureLinearAb->x()) * (NewFloat(b, 1.0f) - textureLinearAb->y());
				auto factorB = textureLinearAb->x() * (NewFloat(b, 1.0f) - textureLinearAb->y());
				auto factorC = (NewFloat(b, 1.0f) - textureLinearAb->x()) * textureLinearAb->y();
				auto factorD = textureLinearAb->x() * textureLinearAb->y();

				textureColor =
				    textureColorA * factorA->xxxx() +
				    textureColorB * factorB->xxxx() +
				    textureColorC * factorC->xxxx() +
				    textureColorD * factorD->xxxx();
			}
			else
			{
				//Point Sampling
				//------------------------------
				auto texelPos = ToInt(textureSt * ToFloat(texSize));
				auto clampTexPos = clampCoordinates(texelPos);
				getTextureColor(clampTexPos, textureColor);
			}

			switch(caps.textureFunction)
			{
			case CGSHandler::TEX0_FUNCTION_MODULATE:
				textureColor = textureColor * inputColor * NewFloat4(b, 2, 2, 2, 2);
				textureColor = Clamp(textureColor, NewFloat4(b, 0, 0, 0, 0), NewFloat4(b, 1, 1, 1, 1));
				if(!caps.textureHasAlpha)
				{
					textureColor = NewFloat4(textureColor->xyz(), inputColor->w());
				}
				break;
			case CGSHandler::TEX0_FUNCTION_DECAL:
				//Nothing to do
				break;
			case CGSHandler::TEX0_FUNCTION_HIGHLIGHT:
			{
				auto tempColor = (textureColor->xyz() * inputColor->xyz() * NewFloat3(b, 2, 2, 2)) + inputColor->www();
				if(caps.textureHasAlpha)
				{
					textureColor = NewFloat4(tempColor, inputColor->w() + textureColor->w());
				}
				else
				{
					textureColor = NewFloat4(tempColor, inputColor->w());
				}
				textureColor = Clamp(textureColor, NewFloat4(b, 0, 0, 0, 0), NewFloat4(b, 1, 1, 1, 1));
			}
			break;
			case CGSHandler::TEX0_FUNCTION_HIGHLIGHT2:
			{
				auto tempColor = (textureColor->xyz() * inputColor->xyz() * NewFloat3(b, 2, 2, 2)) + inputColor->www();
				if(caps.textureHasAlpha)
				{
					textureColor = NewFloat4(tempColor, textureColor->w());
				}
				else
				{
					textureColor = NewFloat4(tempColor, inputColor->w());
				}
				textureColor = Clamp(textureColor, NewFloat4(b, 0, 0, 0, 0), NewFloat4(b, 1, 1, 1, 1));
			}
			break;
			default:
				assert(false);
				break;
			}
		}
		else
		{
			textureColor = inputColor->xyzw();
		}

		auto writeColor = CBoolLvalue(b.CreateVariableBool("writeColor"));
		auto writeDepth = CBoolLvalue(b.CreateVariableBool("writeDepth"));
		auto writeAlpha = CBoolLvalue(b.CreateVariableBool("writeAlpha"));

		writeColor = NewBool(b, true);
		writeDepth = NewBool(b, true);
		writeAlpha = NewBool(b, true);

		if(caps.hasFog)
		{
			auto fogMixColor = Mix(textureColor->xyz(), fogColor->xyz(), inputFog->xxx());
			textureColor = NewFloat4(fogMixColor, textureColor->w());
		}

		auto screenPos = ToInt(inputPosition->xy());

		switch(caps.scanMask)
		{
		default:
			assert(false);
			[[fallthrough]];
		case 0:
			break;
		case 2:
		{
			auto write = (screenPos->y() & NewInt(b, 1)) != NewInt(b, 0);
			writeColor = write;
			writeDepth = write;
		}
		break;
		case 3:
		{
			auto write = (screenPos->y() & NewInt(b, 1)) == NewInt(b, 0);
			writeColor = write;
			writeDepth = write;
		}
		break;
		}

		auto srcIColor = CInt4Lvalue(b.CreateVariableInt("srcIColor"));
		srcIColor = ToInt(textureColor->xyzw() * NewFloat4(b, 255.f, 255.f, 255.f, 255.f));

		CDrawUtils::AlphaTest(b, caps.alphaTestFunction, caps.alphaTestFailAction, srcIColor, alphaRef,
		                      writeColor, writeDepth, writeAlpha);

		bool canDiscardAlpha =
		    (caps.alphaTestFunction != CGSHandler::ALPHA_TEST_ALWAYS) &&
		    (caps.alphaTestFailAction == CGSHandler::ALPHA_TEST_FAIL_RGBONLY);

		//BeginInvocationInterlock(b);

		auto dstPixel = CUintLvalue(b.CreateVariableUint("dstPixel"));
		auto dstDepth = CUintLvalue(b.CreateVariableUint("dstDepth"));
		auto dstIColor = CInt4Lvalue(b.CreateVariableInt("dstIColor"));

		auto finalPixel = CUintLvalue(b.CreateVariableUint("finalPixel"));
		auto finalDepth = CUintLvalue(b.CreateVariableUint("finalDepth"));
		auto finalIColor = CInt4Lvalue(b.CreateVariableInt("finalIColor"));

		dstPixel = Load(subpassColorInput, NewInt2(b, 0, 0))->x();
		dstDepth = Load(subpassDepthInput, NewInt2(b, 0, 0))->x();

		switch(caps.framebufferFormat)
		{
		default:
			assert(false);
			[[fallthrough]];
		case CGSHandler::PSMCT32:
		{
			dstIColor = CMemoryUtils::PSM32ToIVec4(b, dstPixel);
		}
		break;
		case CGSHandler::PSMCT24:
		{
			dstIColor = CMemoryUtils::PSM32ToIVec4(b, dstPixel);
		}
		break;
		case CGSHandler::PSMCT16:
		case CGSHandler::PSMCT16S:
		{
			dstIColor = CMemoryUtils::PSM16ToIVec4(b, dstPixel);
		}
		break;
		}

		if(caps.hasDstAlphaTest)
		{
			CDrawUtils::DestinationAlphaTest(b, caps.framebufferFormat, caps.dstAlphaTestRef, dstPixel, writeColor, writeDepth);
		}

		auto depthTestResult = CBoolLvalue(b.CreateVariableBool("depthTestResult"));
		switch(caps.depthTestFunction)
		{
		case CGSHandler::DEPTH_TEST_ALWAYS:
			depthTestResult = NewBool(b, true);
			break;
		case CGSHandler::DEPTH_TEST_NEVER:
			depthTestResult = NewBool(b, false);
			break;
		case CGSHandler::DEPTH_TEST_GEQUAL:
			depthTestResult = srcDepth >= dstDepth;
			break;
		case CGSHandler::DEPTH_TEST_GREATER:
			depthTestResult = srcDepth > dstDepth;
			break;
		}

		BeginIf(b, !depthTestResult);
		{
			writeColor = NewBool(b, false);
			writeDepth = NewBool(b, false);
		}
		EndIf(b);

		if(caps.hasAlphaBlending)
		{
			//Blend
			auto alphaA = CDrawUtils::GetAlphaABD(b, caps.alphaA, srcIColor, dstIColor);
			auto alphaB = CDrawUtils::GetAlphaABD(b, caps.alphaB, srcIColor, dstIColor);
			auto alphaC = CDrawUtils::GetAlphaC(b, caps.alphaC, srcIColor, dstIColor, alphaFix);
			auto alphaD = CDrawUtils::GetAlphaABD(b, caps.alphaD, srcIColor, dstIColor);

			auto blendedRGB = ShiftRightArithmetic((alphaA - alphaB) * alphaC, NewInt3(b, 7, 7, 7)) + alphaD;
			auto blendedIColor = NewInt4(blendedRGB, srcIColor->w());
			if(caps.colClamp)
			{
				finalIColor = Clamp(blendedIColor, NewInt4(b, 0, 0, 0, 0), NewInt4(b, 255, 255, 255, 255));
			}
			else
			{
				finalIColor = blendedIColor & NewInt4(b, 255, 255, 255, 255);
			}
		}
		else
		{
			finalIColor = srcIColor->xyzw();
		}

		if(caps.fba)
		{
			finalIColor = NewInt4(finalIColor->xyz(), finalIColor->w() | NewInt(b, 0x80));
		}

		if(canDiscardAlpha)
		{
			BeginIf(b, !writeAlpha);
			{
				finalIColor = NewInt4(finalIColor->xyz(), dstIColor->w());
			}
			EndIf(b);
		}

		switch(caps.framebufferFormat)
		{
		default:
			assert(false);
			[[fallthrough]];
		case CGSHandler::PSMCT32:
		case CGSHandler::PSMCT24:
		{
			finalPixel = CMemoryUtils::IVec4ToPSM32(b, finalIColor);
		}
		break;
		case CGSHandler::PSMCT16:
		case CGSHandler::PSMCT16S:
		{
			finalPixel = CMemoryUtils::IVec4ToPSM16(b, finalIColor);
		}
		break;
		}

		finalPixel = (finalPixel & fbWriteMask) | (dstPixel & ~fbWriteMask);

		BeginIf(b, !writeColor);
		{
			finalPixel = dstPixel->x();
		}
		EndIf(b);

		finalDepth = srcDepth->x();
		BeginIf(b, !writeDepth);
		{
			finalDepth = dstDepth->x();
		}
		EndIf(b);

		if(!caps.writeDepth)
		{
			finalDepth = dstDepth->x();
		}

		//EndInvocationInterlock(b);

		outputColor = finalPixel->x();
		outputDepth = finalDepth->x();
	}

	Framework::CMemStream shaderStream;
	Nuanceur::CSpirvShaderGenerator::Generate(shaderStream, b, Nuanceur::CSpirvShaderGenerator::SHADER_TYPE_FRAGMENT);
	shaderStream.Seek(0, Framework::STREAM_SEEK_SET);
	return Framework::Vulkan::CShaderModule(m_context->device, shaderStream);
}

CDrawMobile::PIPELINE_CAPS CDrawMobile::MakeLoadStorePipelineCaps(const PIPELINE_CAPS& caps)
{
	auto strippedCaps = make_convertible<PIPELINE_CAPS>(0);
	strippedCaps.framebufferFormat = caps.framebufferFormat;
	strippedCaps.depthbufferFormat = caps.depthbufferFormat;
	strippedCaps.writeDepth = caps.writeDepth;
	return strippedCaps;
}

PIPELINE CDrawMobile::CreateLoadPipeline(const PIPELINE_CAPS& caps)
{
	PIPELINE loadPipeline;

	auto vertexShader = CreateLoadStoreVertexShader();
	auto fragmentShader = CreateLoadFragmentShader(caps);

	auto result = VK_SUCCESS;

	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;

		{
			VkDescriptorSetLayoutBinding setLayoutBinding = {};
			setLayoutBinding.binding = DESCRIPTOR_LOCATION_BUFFER_MEMORY;
			setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			setLayoutBinding.descriptorCount = 1;
			setLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			setLayoutBindings.push_back(setLayoutBinding);
		}

		{
			VkDescriptorSetLayoutBinding setLayoutBinding = {};
			setLayoutBinding.binding = DESCRIPTOR_LOCATION_IMAGE_SWIZZLETABLE_FB;
			setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			setLayoutBinding.descriptorCount = 1;
			setLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			setLayoutBindings.push_back(setLayoutBinding);
		}

		{
			VkDescriptorSetLayoutBinding setLayoutBinding = {};
			setLayoutBinding.binding = DESCRIPTOR_LOCATION_IMAGE_SWIZZLETABLE_DEPTH;
			setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			setLayoutBinding.descriptorCount = 1;
			setLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			setLayoutBindings.push_back(setLayoutBinding);
		}

		{
			VkDescriptorSetLayoutBinding setLayoutBinding = {};
			setLayoutBinding.binding = DESCRIPTOR_LOCATION_IMAGE_INPUT_COLOR;
			setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			setLayoutBinding.descriptorCount = 1;
			setLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			setLayoutBindings.push_back(setLayoutBinding);
		}

		{
			VkDescriptorSetLayoutBinding setLayoutBinding = {};
			setLayoutBinding.binding = DESCRIPTOR_LOCATION_IMAGE_INPUT_DEPTH;
			setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			setLayoutBinding.descriptorCount = 1;
			setLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			setLayoutBindings.push_back(setLayoutBinding);
		}

		auto setLayoutCreateInfo = Framework::Vulkan::DescriptorSetLayoutCreateInfo();
		setLayoutCreateInfo.bindingCount = static_cast<uint32>(setLayoutBindings.size());
		setLayoutCreateInfo.pBindings = setLayoutBindings.data();

		result = m_context->device.vkCreateDescriptorSetLayout(m_context->device, &setLayoutCreateInfo, nullptr, &loadPipeline.descriptorSetLayout);
		CHECKVULKANERROR(result);
	}

	{
		VkPushConstantRange pushConstantInfo = {};
		pushConstantInfo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantInfo.offset = 0;
		pushConstantInfo.size = sizeof(DRAW_PIPELINE_PUSHCONSTANTS);

		auto pipelineLayoutCreateInfo = Framework::Vulkan::PipelineLayoutCreateInfo();
		pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pSetLayouts = &loadPipeline.descriptorSetLayout;

		result = m_context->device.vkCreatePipelineLayout(m_context->device, &pipelineLayoutCreateInfo, nullptr, &loadPipeline.pipelineLayout);
		CHECKVULKANERROR(result);
	}

	auto inputAssemblyInfo = Framework::Vulkan::PipelineInputAssemblyStateCreateInfo();
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	auto vertexInputInfo = Framework::Vulkan::PipelineVertexInputStateCreateInfo();

	auto rasterStateInfo = Framework::Vulkan::PipelineRasterizationStateCreateInfo();
	rasterStateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterStateInfo.cullMode = VK_CULL_MODE_NONE;
	rasterStateInfo.lineWidth = 1.0f;

	// Our attachment will write to all color channels, but no blending is enabled.
	std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates;
	for(uint32 i = 0; i < 2; i++)
	{
		VkPipelineColorBlendAttachmentState blendAttachmentState = {};
		blendAttachmentState.colorWriteMask = 0xf;
		blendAttachmentStates.push_back(blendAttachmentState);
	}

	auto colorBlendStateInfo = Framework::Vulkan::PipelineColorBlendStateCreateInfo();
	colorBlendStateInfo.attachmentCount = blendAttachmentStates.size();
	colorBlendStateInfo.pAttachments = blendAttachmentStates.data();

	auto viewportStateInfo = Framework::Vulkan::PipelineViewportStateCreateInfo();
	viewportStateInfo.viewportCount = 1;
	viewportStateInfo.scissorCount = 1;

	auto depthStencilStateInfo = Framework::Vulkan::PipelineDepthStencilStateCreateInfo();
	depthStencilStateInfo.depthTestEnable = 1;
	depthStencilStateInfo.depthCompareOp = VK_COMPARE_OP_ALWAYS;
	depthStencilStateInfo.depthWriteEnable = 1;

	auto multisampleStateInfo = Framework::Vulkan::PipelineMultisampleStateCreateInfo();
	multisampleStateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	static const VkDynamicState dynamicStates[] =
	    {
	        VK_DYNAMIC_STATE_VIEWPORT,
	        VK_DYNAMIC_STATE_SCISSOR,
	    };
	auto dynamicStateInfo = Framework::Vulkan::PipelineDynamicStateCreateInfo();
	dynamicStateInfo.pDynamicStates = dynamicStates;
	dynamicStateInfo.dynamicStateCount = sizeof(dynamicStates) / sizeof(dynamicStates[0]);

	VkPipelineShaderStageCreateInfo shaderStages[2] =
	    {
	        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO},
	        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO},
	    };

	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = vertexShader;
	shaderStages[0].pName = "main";
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = fragmentShader;
	shaderStages[1].pName = "main";

	auto pipelineCreateInfo = Framework::Vulkan::GraphicsPipelineCreateInfo();
	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.subpass = 0;
	pipelineCreateInfo.pStages = shaderStages;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyInfo;
	pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
	pipelineCreateInfo.pRasterizationState = &rasterStateInfo;
	pipelineCreateInfo.pColorBlendState = &colorBlendStateInfo;
	pipelineCreateInfo.pViewportState = &viewportStateInfo;
	pipelineCreateInfo.pDepthStencilState = &depthStencilStateInfo;
	pipelineCreateInfo.pMultisampleState = &multisampleStateInfo;
	pipelineCreateInfo.pDynamicState = &dynamicStateInfo;
	pipelineCreateInfo.renderPass = m_renderPass;
	pipelineCreateInfo.layout = loadPipeline.pipelineLayout;

	result = m_context->device.vkCreateGraphicsPipelines(m_context->device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &loadPipeline.pipeline);
	CHECKVULKANERROR(result);

	return loadPipeline;
}

PIPELINE CDrawMobile::CreateStorePipeline(const PIPELINE_CAPS& caps)
{
	PIPELINE storePipeline;

	auto vertexShader = CreateLoadStoreVertexShader();
	auto fragmentShader = CreateStoreFragmentShader(caps);

	auto result = VK_SUCCESS;

	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;

		{
			VkDescriptorSetLayoutBinding setLayoutBinding = {};
			setLayoutBinding.binding = DESCRIPTOR_LOCATION_BUFFER_MEMORY;
			setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			setLayoutBinding.descriptorCount = 1;
			setLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			setLayoutBindings.push_back(setLayoutBinding);
		}

		{
			VkDescriptorSetLayoutBinding setLayoutBinding = {};
			setLayoutBinding.binding = DESCRIPTOR_LOCATION_IMAGE_SWIZZLETABLE_FB;
			setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			setLayoutBinding.descriptorCount = 1;
			setLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			setLayoutBindings.push_back(setLayoutBinding);
		}

		{
			VkDescriptorSetLayoutBinding setLayoutBinding = {};
			setLayoutBinding.binding = DESCRIPTOR_LOCATION_IMAGE_SWIZZLETABLE_DEPTH;
			setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			setLayoutBinding.descriptorCount = 1;
			setLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			setLayoutBindings.push_back(setLayoutBinding);
		}

		{
			VkDescriptorSetLayoutBinding setLayoutBinding = {};
			setLayoutBinding.binding = DESCRIPTOR_LOCATION_IMAGE_INPUT_COLOR;
			setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			setLayoutBinding.descriptorCount = 1;
			setLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			setLayoutBindings.push_back(setLayoutBinding);
		}

		{
			VkDescriptorSetLayoutBinding setLayoutBinding = {};
			setLayoutBinding.binding = DESCRIPTOR_LOCATION_IMAGE_INPUT_DEPTH;
			setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			setLayoutBinding.descriptorCount = 1;
			setLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			setLayoutBindings.push_back(setLayoutBinding);
		}

		auto setLayoutCreateInfo = Framework::Vulkan::DescriptorSetLayoutCreateInfo();
		setLayoutCreateInfo.bindingCount = static_cast<uint32>(setLayoutBindings.size());
		setLayoutCreateInfo.pBindings = setLayoutBindings.data();

		result = m_context->device.vkCreateDescriptorSetLayout(m_context->device, &setLayoutCreateInfo, nullptr, &storePipeline.descriptorSetLayout);
		CHECKVULKANERROR(result);
	}

	{
		VkPushConstantRange pushConstantInfo = {};
		pushConstantInfo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantInfo.offset = 0;
		pushConstantInfo.size = sizeof(DRAW_PIPELINE_PUSHCONSTANTS);

		auto pipelineLayoutCreateInfo = Framework::Vulkan::PipelineLayoutCreateInfo();
		pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pSetLayouts = &storePipeline.descriptorSetLayout;

		result = m_context->device.vkCreatePipelineLayout(m_context->device, &pipelineLayoutCreateInfo, nullptr, &storePipeline.pipelineLayout);
		CHECKVULKANERROR(result);
	}

	auto inputAssemblyInfo = Framework::Vulkan::PipelineInputAssemblyStateCreateInfo();
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	auto vertexInputInfo = Framework::Vulkan::PipelineVertexInputStateCreateInfo();

	auto rasterStateInfo = Framework::Vulkan::PipelineRasterizationStateCreateInfo();
	rasterStateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterStateInfo.cullMode = VK_CULL_MODE_NONE;
	rasterStateInfo.lineWidth = 1.0f;

	// Our attachment will write to all color channels, but no blending is enabled.
	std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates;
	for(uint32 i = 0; i < 2; i++)
	{
		VkPipelineColorBlendAttachmentState blendAttachmentState = {};
		blendAttachmentState.colorWriteMask = 0xf;
		blendAttachmentStates.push_back(blendAttachmentState);
	}

	auto colorBlendStateInfo = Framework::Vulkan::PipelineColorBlendStateCreateInfo();
	colorBlendStateInfo.attachmentCount = blendAttachmentStates.size();
	colorBlendStateInfo.pAttachments = blendAttachmentStates.data();

	auto viewportStateInfo = Framework::Vulkan::PipelineViewportStateCreateInfo();
	viewportStateInfo.viewportCount = 1;
	viewportStateInfo.scissorCount = 1;

	auto depthStencilStateInfo = Framework::Vulkan::PipelineDepthStencilStateCreateInfo();
	depthStencilStateInfo.depthTestEnable = 1;
	depthStencilStateInfo.depthCompareOp = VK_COMPARE_OP_ALWAYS;
	depthStencilStateInfo.depthWriteEnable = 1;

	auto multisampleStateInfo = Framework::Vulkan::PipelineMultisampleStateCreateInfo();
	multisampleStateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	static const VkDynamicState dynamicStates[] =
	    {
	        VK_DYNAMIC_STATE_VIEWPORT,
	        VK_DYNAMIC_STATE_SCISSOR,
	    };
	auto dynamicStateInfo = Framework::Vulkan::PipelineDynamicStateCreateInfo();
	dynamicStateInfo.pDynamicStates = dynamicStates;
	dynamicStateInfo.dynamicStateCount = sizeof(dynamicStates) / sizeof(dynamicStates[0]);

	VkPipelineShaderStageCreateInfo shaderStages[2] =
	    {
	        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO},
	        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO},
	    };

	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = vertexShader;
	shaderStages[0].pName = "main";
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = fragmentShader;
	shaderStages[1].pName = "main";

	auto pipelineCreateInfo = Framework::Vulkan::GraphicsPipelineCreateInfo();
	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.subpass = 2;
	pipelineCreateInfo.pStages = shaderStages;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyInfo;
	pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
	pipelineCreateInfo.pRasterizationState = &rasterStateInfo;
	pipelineCreateInfo.pColorBlendState = &colorBlendStateInfo;
	pipelineCreateInfo.pViewportState = &viewportStateInfo;
	pipelineCreateInfo.pDepthStencilState = &depthStencilStateInfo;
	pipelineCreateInfo.pMultisampleState = &multisampleStateInfo;
	pipelineCreateInfo.pDynamicState = &dynamicStateInfo;
	pipelineCreateInfo.renderPass = m_renderPass;
	pipelineCreateInfo.layout = storePipeline.pipelineLayout;

	result = m_context->device.vkCreateGraphicsPipelines(m_context->device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &storePipeline.pipeline);
	CHECKVULKANERROR(result);

	return storePipeline;
}

Framework::Vulkan::CShaderModule CDrawMobile::CreateLoadStoreVertexShader()
{
	using namespace Nuanceur;

	auto b = CShaderBuilder();

	{
		//Vertex Inputs
		auto vertexIndex = CIntLvalue(b.CreateInputInt(Nuanceur::SEMANTIC_SYSTEM_VERTEXINDEX));

		//Outputs
		auto outputPosition = CFloat4Lvalue(b.CreateOutput(Nuanceur::SEMANTIC_SYSTEM_POSITION));

		auto position = NewFloat2(
		    ToFloat(vertexIndex << NewInt(b, 1) & NewInt(b, 2)),
		    ToFloat(vertexIndex & NewInt(b, 2)));
		outputPosition = NewFloat4(
		    position->x() * NewFloat(b, 2) + NewFloat(b, -1),
		    position->y() * NewFloat(b, 2) + NewFloat(b, -1),
		    NewFloat(b, 0),
		    NewFloat(b, 1));
	}

	Framework::CMemStream shaderStream;
	Nuanceur::CSpirvShaderGenerator::Generate(shaderStream, b, Nuanceur::CSpirvShaderGenerator::SHADER_TYPE_VERTEX);
	shaderStream.Seek(0, Framework::STREAM_SEEK_SET);
	return Framework::Vulkan::CShaderModule(m_context->device, shaderStream);
}

Framework::Vulkan::CShaderModule CDrawMobile::CreateLoadFragmentShader(const PIPELINE_CAPS& caps)
{
	using namespace Nuanceur;

	auto b = CShaderBuilder();

	{
		//Inputs
		auto inputPosition = CFloat4Lvalue(b.CreateInput(Nuanceur::SEMANTIC_SYSTEM_POSITION));

		//Outputs
		auto outputColor = CUintLvalue(b.CreateOutputUint(Nuanceur::SEMANTIC_SYSTEM_COLOR, 0));
		auto outputDepth = CUintLvalue(b.CreateOutputUint(Nuanceur::SEMANTIC_SYSTEM_COLOR, 1));

		auto memoryBuffer = CArrayUintValue(b.CreateUniformArrayUint("memoryBuffer", DESCRIPTOR_LOCATION_BUFFER_MEMORY, Nuanceur::SYMBOL_ATTRIBUTE_COHERENT));
		auto fbSwizzleTable = CImageUint2DValue(b.CreateImage2DUint(DESCRIPTOR_LOCATION_IMAGE_SWIZZLETABLE_FB));
		auto depthSwizzleTable = CImageUint2DValue(b.CreateImage2DUint(DESCRIPTOR_LOCATION_IMAGE_SWIZZLETABLE_DEPTH));

		//Push constants
		auto fbDepthParams = CInt4Lvalue(b.CreateUniformInt4("fbDepthParams", Nuanceur::UNIFORM_UNIT_PUSHCONSTANT));

		auto fbBufAddress = fbDepthParams->x();
		auto fbBufWidth = fbDepthParams->y();
		auto depthBufAddress = fbDepthParams->z();
		auto depthBufWidth = fbDepthParams->w();

		auto dstPixel = CUintLvalue(b.CreateVariableUint("dstPixel"));
		auto dstDepth = CUintLvalue(b.CreateVariableUint("dstDepth"));

		auto fbAddress = CIntLvalue(b.CreateTemporaryInt());
		auto depthAddress = CIntLvalue(b.CreateTemporaryInt());

		auto screenPos = ToInt(inputPosition->xy());

		switch(caps.framebufferFormat)
		{
		default:
			assert(false);
		case CGSHandler::PSMCT32:
		case CGSHandler::PSMCT24:
		case CGSHandler::PSMZ24:
			fbAddress = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMCT32>(
			    b, fbSwizzleTable, fbBufAddress, fbBufWidth, screenPos);
			break;
		case CGSHandler::PSMCT16:
		case CGSHandler::PSMCT16S:
			fbAddress = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMCT16>(
			    b, fbSwizzleTable, fbBufAddress, fbBufWidth, screenPos);
			break;
		}

		switch(caps.depthbufferFormat)
		{
		default:
			assert(false);
		case CGSHandler::PSMZ32:
		case CGSHandler::PSMZ24:
			depthAddress = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMZ32>(
			    b, depthSwizzleTable, depthBufAddress, depthBufWidth, screenPos);
			break;
		case CGSHandler::PSMZ16:
		case CGSHandler::PSMZ16S:
			depthAddress = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMZ16>(
			    b, depthSwizzleTable, depthBufAddress, depthBufWidth, screenPos);
			break;
		}

		//Prevent writing out of bounds (seems to cause wierd issues
		//on Intel GPUs with games such as SNK vs. Capcom: SVC Chaos)
		fbAddress = fbAddress & NewInt(b, CGSHandler::RAMSIZE - 1);
		depthAddress = depthAddress & NewInt(b, CGSHandler::RAMSIZE - 1);

		switch(caps.framebufferFormat)
		{
		default:
			assert(false);
		case CGSHandler::PSMCT32:
		{
			dstPixel = CMemoryUtils::Memory_Read32(b, memoryBuffer, fbAddress);
		}
		break;
		case CGSHandler::PSMCT24:
		{
			dstPixel = CMemoryUtils::Memory_Read24(b, memoryBuffer, fbAddress);
		}
		break;
		case CGSHandler::PSMCT16:
		case CGSHandler::PSMCT16S:
		{
			dstPixel = CMemoryUtils::Memory_Read16(b, memoryBuffer, fbAddress);
		}
		break;
		}

		dstDepth = CDrawUtils::GetDepth(b, caps.depthbufferFormat, depthAddress, memoryBuffer);

		outputColor = dstPixel->x();
		outputDepth = dstDepth->x();
	}

	Framework::CMemStream shaderStream;
	Nuanceur::CSpirvShaderGenerator::Generate(shaderStream, b, Nuanceur::CSpirvShaderGenerator::SHADER_TYPE_FRAGMENT);
	shaderStream.Seek(0, Framework::STREAM_SEEK_SET);
	return Framework::Vulkan::CShaderModule(m_context->device, shaderStream);
}

Framework::Vulkan::CShaderModule CDrawMobile::CreateStoreFragmentShader(const PIPELINE_CAPS& caps)
{
	using namespace Nuanceur;

	auto b = CShaderBuilder();

	{
		//Inputs
		auto inputPosition = CFloat4Lvalue(b.CreateInput(Nuanceur::SEMANTIC_SYSTEM_POSITION));

		//Outputs
		auto outputColor = CUintLvalue(b.CreateOutputUint(Nuanceur::SEMANTIC_SYSTEM_COLOR, 0));
		auto outputDepth = CUintLvalue(b.CreateOutputUint(Nuanceur::SEMANTIC_SYSTEM_COLOR, 1));

		auto memoryBuffer = CArrayUintValue(b.CreateUniformArrayUint("memoryBuffer", DESCRIPTOR_LOCATION_BUFFER_MEMORY, Nuanceur::SYMBOL_ATTRIBUTE_COHERENT));
		auto fbSwizzleTable = CImageUint2DValue(b.CreateImage2DUint(DESCRIPTOR_LOCATION_IMAGE_SWIZZLETABLE_FB));
		auto depthSwizzleTable = CImageUint2DValue(b.CreateImage2DUint(DESCRIPTOR_LOCATION_IMAGE_SWIZZLETABLE_DEPTH));
		auto subpassColorInput = CSubpassInputUintValue(b.CreateSubpassInputUint(DESCRIPTOR_LOCATION_IMAGE_INPUT_COLOR, 0));
		auto subpassDepthInput = CSubpassInputUintValue(b.CreateSubpassInputUint(DESCRIPTOR_LOCATION_IMAGE_INPUT_DEPTH, 1));

		//Push constants
		auto fbDepthParams = CInt4Lvalue(b.CreateUniformInt4("fbDepthParams", Nuanceur::UNIFORM_UNIT_PUSHCONSTANT));
		auto texParams0 = CInt4Lvalue(b.CreateUniformInt4("texParams0", Nuanceur::UNIFORM_UNIT_PUSHCONSTANT));
		auto texParams1 = CInt4Lvalue(b.CreateUniformInt4("texParams1", Nuanceur::UNIFORM_UNIT_PUSHCONSTANT));
		auto texParams2 = CInt4Lvalue(b.CreateUniformInt4("texParams2", Nuanceur::UNIFORM_UNIT_PUSHCONSTANT));
		auto alphaFbParams = CInt4Lvalue(b.CreateUniformInt4("alphaFbParams", Nuanceur::UNIFORM_UNIT_PUSHCONSTANT));
		auto fogColor = CFloat4Lvalue(b.CreateUniformFloat4("fogColor", Nuanceur::UNIFORM_UNIT_PUSHCONSTANT));

		auto fbBufAddress = fbDepthParams->x();
		auto fbBufWidth = fbDepthParams->y();
		auto depthBufAddress = fbDepthParams->z();
		auto depthBufWidth = fbDepthParams->w();

		auto dstPixel = CUintLvalue(b.CreateVariableUint("dstPixel"));
		auto dstDepth = CUintLvalue(b.CreateVariableUint("dstDepth"));

		dstPixel = Load(subpassColorInput, NewInt2(b, 0, 0))->x();
		dstDepth = Load(subpassDepthInput, NewInt2(b, 0, 0))->x();

		auto fbAddress = CIntLvalue(b.CreateTemporaryInt());
		auto depthAddress = CIntLvalue(b.CreateTemporaryInt());

		auto screenPos = ToInt(inputPosition->xy());

		switch(caps.framebufferFormat)
		{
		default:
			assert(false);
		case CGSHandler::PSMCT32:
		case CGSHandler::PSMCT24:
		case CGSHandler::PSMZ24:
			fbAddress = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMCT32>(
			    b, fbSwizzleTable, fbBufAddress, fbBufWidth, screenPos);
			break;
		case CGSHandler::PSMCT16:
		case CGSHandler::PSMCT16S:
			fbAddress = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMCT16>(
			    b, fbSwizzleTable, fbBufAddress, fbBufWidth, screenPos);
			break;
		}

		switch(caps.depthbufferFormat)
		{
		default:
			assert(false);
		case CGSHandler::PSMZ32:
		case CGSHandler::PSMZ24:
			depthAddress = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMZ32>(
			    b, depthSwizzleTable, depthBufAddress, depthBufWidth, screenPos);
			break;
		case CGSHandler::PSMZ16:
		case CGSHandler::PSMZ16S:
			depthAddress = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMZ16>(
			    b, depthSwizzleTable, depthBufAddress, depthBufWidth, screenPos);
			break;
		}

		//Prevent writing out of bounds (seems to cause wierd issues
		//on Intel GPUs with games such as SNK vs. Capcom: SVC Chaos)
		fbAddress = fbAddress & NewInt(b, CGSHandler::RAMSIZE - 1);
		depthAddress = depthAddress & NewInt(b, CGSHandler::RAMSIZE - 1);

		CDrawUtils::WriteToFramebuffer(b, caps.framebufferFormat, memoryBuffer, fbAddress, dstPixel);
		if(caps.writeDepth)
		{
			CDrawUtils::WriteToDepthbuffer(b, caps.depthbufferFormat, memoryBuffer, depthAddress, dstDepth);
		}

		outputColor = dstPixel->x();
	}

	Framework::CMemStream shaderStream;
	Nuanceur::CSpirvShaderGenerator::Generate(shaderStream, b, Nuanceur::CSpirvShaderGenerator::SHADER_TYPE_FRAGMENT);
	shaderStream.Seek(0, Framework::STREAM_SEEK_SET);
	return Framework::Vulkan::CShaderModule(m_context->device, shaderStream);
}

void CDrawMobile::CreateDrawImages()
{
	m_drawColorImage = Framework::Vulkan::CImage(m_context->device, m_context->physicalDeviceMemoryProperties,
	                                             VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
	                                             VK_FORMAT_R32_UINT, DRAW_AREA_SIZE, DRAW_AREA_SIZE, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_drawColorImage.SetLayout(m_context->queue, m_context->commandBufferPool,
	                           VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

	m_drawColorImageView = m_drawColorImage.CreateImageView();

	m_drawDepthImage = Framework::Vulkan::CImage(m_context->device, m_context->physicalDeviceMemoryProperties,
	                                             VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
	                                             VK_FORMAT_R32_UINT, DRAW_AREA_SIZE, DRAW_AREA_SIZE, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_drawDepthImage.SetLayout(m_context->queue, m_context->commandBufferPool,
	                           VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

	m_drawDepthImageView = m_drawDepthImage.CreateImageView();
}
