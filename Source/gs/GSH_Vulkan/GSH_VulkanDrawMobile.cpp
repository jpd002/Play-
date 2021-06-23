#include "GSH_VulkanDrawMobile.h"
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
    : m_context(context)
    , m_frameCommandBuffer(frameCommandBuffer)
    , m_drawPipelineCache(context->device)
{
	CreateRenderPass();
	CreateDrawImages();
	CreateFramebuffer();

	m_loadPipeline = CreateLoadPipeline();
	m_storePipeline = CreateStorePipeline();

	for(auto& frame : m_frames)
	{
		frame.vertexBuffer = Framework::Vulkan::CBuffer(
		    m_context->device, m_context->physicalDeviceMemoryProperties,
		    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		    sizeof(PRIM_VERTEX) * MAX_VERTEX_COUNT);

		auto result = m_context->device.vkMapMemory(m_context->device, frame.vertexBuffer.GetMemory(),
		                                            0, VK_WHOLE_SIZE, 0, reinterpret_cast<void**>(&frame.vertexBufferPtr));
		CHECKVULKANERROR(result);
	}

	m_pipelineCaps <<= 0;
}

CDrawMobile::~CDrawMobile()
{
	for(auto& frame : m_frames)
	{
		m_context->device.vkUnmapMemory(m_context->device, frame.vertexBuffer.GetMemory());
	}
	m_context->device.vkDestroyFramebuffer(m_context->device, m_framebuffer, nullptr);
	m_context->device.vkDestroyRenderPass(m_context->device, m_renderPass, nullptr);
	m_context->device.vkDestroyImageView(m_context->device, m_drawColorImageView, nullptr);
}

void CDrawMobile::SetPipelineCaps(const PIPELINE_CAPS& caps)
{
	bool changed = static_cast<uint64>(caps) != static_cast<uint64>(m_pipelineCaps);
	if(!changed) return;
	if(caps.textureUseMemoryCopy)
	{
		FlushRenderPass();
	}
	FlushVertices();
	m_pipelineCaps = caps;
}

void CDrawMobile::SetFramebufferParams(uint32 addr, uint32 width, uint32 writeMask)
{
	bool changed =
	    (m_loadStorePushConstants.fbBufAddr != addr) ||
	    (m_loadStorePushConstants.fbBufWidth != width) ||
	    (m_drawPushConstants.fbWriteMask != writeMask);
	if(!changed) return;
	FlushRenderPass();
	m_loadStorePushConstants.fbBufAddr = addr;
	m_loadStorePushConstants.fbBufWidth = width;
	m_drawPushConstants.fbWriteMask = writeMask;
}

void CDrawMobile::SetDepthbufferParams(uint32 addr, uint32 width)
{
	bool changed =
	    (m_loadStorePushConstants.depthBufAddr != addr) ||
	    (m_loadStorePushConstants.depthBufWidth != width);
	if(!changed) return;
	FlushRenderPass();
	m_loadStorePushConstants.depthBufAddr = addr;
	m_loadStorePushConstants.depthBufWidth = width;
}

void CDrawMobile::SetTextureParams(uint32 bufAddr, uint32 bufWidth, uint32 width, uint32 height, uint32 csa)
{
	bool changed =
	    (m_drawPushConstants.texBufAddr != bufAddr) ||
	    (m_drawPushConstants.texBufWidth != bufWidth) ||
	    (m_drawPushConstants.texWidth != width) ||
	    (m_drawPushConstants.texHeight != height) ||
	    (m_drawPushConstants.texCsa != csa);
	if(!changed) return;
	FlushVertices();
	m_drawPushConstants.texBufAddr = bufAddr;
	m_drawPushConstants.texBufWidth = bufWidth;
	m_drawPushConstants.texWidth = width;
	m_drawPushConstants.texHeight = height;
	m_drawPushConstants.texCsa = csa;
}

void CDrawMobile::SetClutBufferOffset(uint32 clutBufferOffset)
{
	bool changed = m_clutBufferOffset != clutBufferOffset;
	if(!changed) return;
	FlushVertices();
	m_clutBufferOffset = clutBufferOffset;
}

void CDrawMobile::SetTextureAlphaParams(uint32 texA0, uint32 texA1)
{
	bool changed =
	    (m_drawPushConstants.texA0 != texA0) ||
	    (m_drawPushConstants.texA1 != texA1);
	if(!changed) return;
	FlushVertices();
	m_drawPushConstants.texA0 = texA0;
	m_drawPushConstants.texA1 = texA1;
}

void CDrawMobile::SetTextureClampParams(uint32 clampMinU, uint32 clampMinV, uint32 clampMaxU, uint32 clampMaxV)
{
	bool changed =
	    (m_drawPushConstants.clampMin[0] != clampMinU) ||
	    (m_drawPushConstants.clampMin[1] != clampMinV) ||
	    (m_drawPushConstants.clampMax[0] != clampMaxU) ||
	    (m_drawPushConstants.clampMax[1] != clampMaxV);
	if(!changed) return;
	FlushVertices();
	m_drawPushConstants.clampMin[0] = clampMinU;
	m_drawPushConstants.clampMin[1] = clampMinV;
	m_drawPushConstants.clampMax[0] = clampMaxU;
	m_drawPushConstants.clampMax[1] = clampMaxV;
}

void CDrawMobile::SetFogParams(float fogR, float fogG, float fogB)
{
	bool changed =
	    (m_drawPushConstants.fogColor[0] != fogR) ||
	    (m_drawPushConstants.fogColor[1] != fogG) ||
	    (m_drawPushConstants.fogColor[2] != fogB);
	if(!changed) return;
	FlushVertices();
	m_drawPushConstants.fogColor[0] = fogR;
	m_drawPushConstants.fogColor[1] = fogG;
	m_drawPushConstants.fogColor[2] = fogB;
	m_drawPushConstants.fogColor[3] = 0;
}

void CDrawMobile::SetAlphaTestParams(uint32 alphaRef)
{
	bool changed = (m_drawPushConstants.alphaRef != alphaRef);
	if(!changed) return;
	FlushVertices();
	m_drawPushConstants.alphaRef = alphaRef;
}

void CDrawMobile::SetAlphaBlendingParams(uint32 alphaFix)
{
	bool changed =
	    (m_drawPushConstants.alphaFix != alphaFix);
	if(!changed) return;
	FlushVertices();
	m_drawPushConstants.alphaFix = alphaFix;
}

void CDrawMobile::SetScissor(uint32 scissorX, uint32 scissorY, uint32 scissorWidth, uint32 scissorHeight)
{
	bool changed =
	    (m_scissorX != scissorX) ||
	    (m_scissorY != scissorY) ||
	    (m_scissorWidth != scissorWidth) ||
	    (m_scissorHeight != scissorHeight);
	if(!changed) return;
	FlushVertices();
	m_scissorX = scissorX;
	m_scissorY = scissorY;
	m_scissorWidth = scissorWidth;
	m_scissorHeight = scissorHeight;
}

void CDrawMobile::SetMemoryCopyParams(uint32 memoryCopyAddress, uint32 memoryCopySize)
{
}

void CDrawMobile::AddVertices(const PRIM_VERTEX* vertexBeginPtr, const PRIM_VERTEX* vertexEndPtr)
{
	auto amount = vertexEndPtr - vertexBeginPtr;
	if((m_passVertexEnd + amount) > MAX_VERTEX_COUNT)
	{
		m_frameCommandBuffer->Flush();
		assert((m_passVertexEnd + amount) <= MAX_VERTEX_COUNT);
	}
	auto& frame = m_frames[m_frameCommandBuffer->GetCurrentFrame()];
	memcpy(frame.vertexBufferPtr + m_passVertexEnd, vertexBeginPtr, amount * sizeof(PRIM_VERTEX));
	m_passVertexEnd += amount;
}

void CDrawMobile::FlushVertices()
{
	uint32 vertexCount = m_passVertexEnd - m_passVertexStart;
	if(vertexCount == 0) return;

	auto& frame = m_frames[m_frameCommandBuffer->GetCurrentFrame()];
	auto commandBuffer = m_frameCommandBuffer->GetCommandBuffer();

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
		auto descriptorSetCaps = make_convertible<DESCRIPTORSET_CAPS>(0);
		descriptorSetCaps.framebufferFormat = CGSHandler::PSMCT32;
		descriptorSetCaps.depthbufferFormat = CGSHandler::PSMZ32;

		auto descriptorSet = PrepareDescriptorSet(m_loadPipeline.descriptorSetLayout, descriptorSetCaps);

		m_context->device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_loadPipeline.pipelineLayout,
		                                          0, 1, &descriptorSet, 0, nullptr);
		m_context->device.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_loadPipeline.pipeline);
		m_context->device.vkCmdPushConstants(commandBuffer, m_loadPipeline.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
		                                     0, sizeof(LOAD_STORE_PIPELINE_PUSHCONSTANTS), &m_loadStorePushConstants);
		m_context->device.vkCmdDraw(commandBuffer, 3, 1, 0, 0);

		m_context->device.vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);

		m_renderPassBegun = true;
	}

	//Find pipeline and create it if we've never encountered it before
	auto drawPipeline = m_drawPipelineCache.TryGetPipeline(m_pipelineCaps);
	if(!drawPipeline)
	{
		drawPipeline = m_drawPipelineCache.RegisterPipeline(m_pipelineCaps, CreateDrawPipeline(m_pipelineCaps));
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
	                                     0, sizeof(DRAW_PIPELINE_PUSHCONSTANTS), &m_drawPushConstants);

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
		auto descriptorSetCaps = make_convertible<DESCRIPTORSET_CAPS>(0);
		descriptorSetCaps.framebufferFormat = CGSHandler::PSMCT32;
		descriptorSetCaps.depthbufferFormat = CGSHandler::PSMZ32;

		auto descriptorSet = PrepareDescriptorSet(m_storePipeline.descriptorSetLayout, descriptorSetCaps);

		m_context->device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_storePipeline.pipelineLayout,
		                                          0, 1, &descriptorSet, 0, nullptr);
		m_context->device.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_storePipeline.pipeline);
		m_context->device.vkCmdPushConstants(commandBuffer, m_storePipeline.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
		                                     0, sizeof(LOAD_STORE_PIPELINE_PUSHCONSTANTS), &m_loadStorePushConstants);
		m_context->device.vkCmdDraw(commandBuffer, 3, 1, 0, 0);

		m_context->device.vkCmdEndRenderPass(commandBuffer);
		m_renderPassBegun = false;
	}
}

void CDrawMobile::PreFlushFrameCommandBuffer()
{
	FlushRenderPass();
}

void CDrawMobile::PostFlushFrameCommandBuffer()
{
	m_passVertexStart = m_passVertexEnd = 0;
}

VkDescriptorSet CDrawMobile::PrepareDescriptorSet(VkDescriptorSetLayout descriptorSetLayout, const DESCRIPTORSET_CAPS& caps)
{
	auto descriptorSetIterator = m_drawDescriptorSetCache.find(caps);
	if(descriptorSetIterator != std::end(m_drawDescriptorSetCache))
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
		descriptorClutBufferInfo.range = VK_WHOLE_SIZE;

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

	m_drawDescriptorSetCache.insert(std::make_pair(caps, descriptorSet));

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

	std::vector<VkAttachmentReference> inputAttachmentRefs;
	inputAttachmentRefs.push_back(colorRef);
	inputAttachmentRefs.push_back(depthRef);

	std::vector<VkSubpassDescription> subpasses;

	//Unswizzle Subpass
	{
		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.pColorAttachments = &colorRef;
		subpass.colorAttachmentCount = 1;
		subpass.pDepthStencilAttachment = &depthRef;
		subpasses.push_back(subpass);
	}

	//Draw Subpass
	{
		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.pColorAttachments = &colorRef;
		subpass.colorAttachmentCount = 1;
		subpass.pDepthStencilAttachment = &depthRef;
		subpass.pInputAttachments = inputAttachmentRefs.data();
		subpass.inputAttachmentCount = inputAttachmentRefs.size();
		subpasses.push_back(subpass);
	}

	//Swizzle pass
	{
		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.pInputAttachments = inputAttachmentRefs.data();
		subpass.inputAttachmentCount = inputAttachmentRefs.size();
		subpasses.push_back(subpass);
	}

	std::vector<VkSubpassDependency> subpassDependencies;

	{
		VkSubpassDependency subpassDependency = {};
		subpassDependency.srcSubpass = 0;
		subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependency.dstSubpass = 1;
		subpassDependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		subpassDependency.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;

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

		subpassDependencies.push_back(subpassDependency);
	}

	std::vector<VkAttachmentDescription> attachments;

	{
		VkAttachmentDescription attachment = {};
		attachment.format = VK_FORMAT_R8G8B8A8_UNORM;
		attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachments.push_back(attachment);
	}

	{
		VkAttachmentDescription attachment = {};
		attachment.format = VK_FORMAT_D32_SFLOAT;
		attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
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

	auto vertexShader = CreateDrawVertexShader();
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
	VkPipelineColorBlendAttachmentState blendAttachment = {};
	blendAttachment.colorWriteMask = 0xf;

	auto colorBlendStateInfo = Framework::Vulkan::PipelineColorBlendStateCreateInfo();
	colorBlendStateInfo.attachmentCount = 1;
	colorBlendStateInfo.pAttachments = &blendAttachment;

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

Framework::Vulkan::CShaderModule CDrawMobile::CreateDrawVertexShader()
{
	using namespace Nuanceur;

	auto b = CShaderBuilder();

	{
		//Vertex Inputs
		auto inputPosition = CFloat4Lvalue(b.CreateInput(Nuanceur::SEMANTIC_POSITION));
		auto inputDepth = CUint4Lvalue(b.CreateInputUint(Nuanceur::SEMANTIC_TEXCOORD, VERTEX_ATTRIB_LOCATION_DEPTH - 1));
		auto inputColor = CFloat4Lvalue(b.CreateInput(Nuanceur::SEMANTIC_TEXCOORD, VERTEX_ATTRIB_LOCATION_COLOR - 1));
		auto inputTexCoord = CFloat4Lvalue(b.CreateInput(Nuanceur::SEMANTIC_TEXCOORD, VERTEX_ATTRIB_LOCATION_TEXCOORD - 1));
		auto inputFog = CFloat4Lvalue(b.CreateInput(Nuanceur::SEMANTIC_TEXCOORD, VERTEX_ATTRIB_LOCATION_FOG - 1));

		//Outputs
		auto outputPosition = CFloat4Lvalue(b.CreateOutput(Nuanceur::SEMANTIC_SYSTEM_POSITION));
		auto outputDepth = CFloat4Lvalue(b.CreateOutput(Nuanceur::SEMANTIC_TEXCOORD, 1));
		auto outputColor = CFloat4Lvalue(b.CreateOutput(Nuanceur::SEMANTIC_TEXCOORD, 2));
		auto outputTexCoord = CFloat4Lvalue(b.CreateOutput(Nuanceur::SEMANTIC_TEXCOORD, 3));
		auto outputFog = CFloat4Lvalue(b.CreateOutput(Nuanceur::SEMANTIC_TEXCOORD, 4));

		auto position = ((inputPosition->xy() + NewFloat2(b, 0.5f, 0.5f)) * NewFloat2(b, 2.f / DRAW_AREA_SIZE, 2.f / DRAW_AREA_SIZE) + NewFloat2(b, -1, -1));

		outputPosition = NewFloat4(position, NewFloat2(b, 0.f, 1.f));
		outputDepth = ToFloat(inputDepth) / NewFloat4(b, DEPTH_MAX, DEPTH_MAX, DEPTH_MAX, DEPTH_MAX);
		outputColor = inputColor->xyzw();
		outputTexCoord = inputTexCoord->xyzw();
		outputFog = inputFog->xyzw();
	}

	Framework::CMemStream shaderStream;
	Nuanceur::CSpirvShaderGenerator::Generate(shaderStream, b, Nuanceur::CSpirvShaderGenerator::SHADER_TYPE_VERTEX);
	shaderStream.Seek(0, Framework::STREAM_SEEK_SET);
	return Framework::Vulkan::CShaderModule(m_context->device, shaderStream);
}

static Nuanceur::CUintRvalue GetDepth(Nuanceur::CShaderBuilder& b, uint32 depthFormat,
                                      Nuanceur::CIntValue depthAddress, Nuanceur::CArrayUintValue memoryBuffer)
{
	switch(depthFormat)
	{
	default:
		assert(false);
	case CGSHandler::PSMZ32:
		return CMemoryUtils::Memory_Read32(b, memoryBuffer, depthAddress);
	case CGSHandler::PSMZ24:
		return CMemoryUtils::Memory_Read24(b, memoryBuffer, depthAddress);
	case CGSHandler::PSMZ16:
	case CGSHandler::PSMZ16S:
		return CMemoryUtils::Memory_Read16(b, memoryBuffer, depthAddress);
	}
}

static Nuanceur::CIntRvalue ClampTexCoord(Nuanceur::CShaderBuilder& b, uint32 clampMode, Nuanceur::CIntValue texCoord, Nuanceur::CIntValue texSize,
                                          Nuanceur::CIntValue clampMin, Nuanceur::CIntValue clampMax)
{
	using namespace Nuanceur;

	switch(clampMode)
	{
	default:
		assert(false);
	case CGSHandler::CLAMP_MODE_REPEAT:
		return texCoord & (texSize - NewInt(b, 1));
	case CGSHandler::CLAMP_MODE_CLAMP:
		return Clamp(texCoord, NewInt(b, 0), texSize - NewInt(b, 1));
	case CGSHandler::CLAMP_MODE_REGION_CLAMP:
		return Clamp(texCoord, clampMin, clampMax);
	case CGSHandler::CLAMP_MODE_REGION_REPEAT:
		return (texCoord & clampMin) | clampMax;
	}
};

static Nuanceur::CFloat4Rvalue GetClutColor(Nuanceur::CShaderBuilder& b,
                                            uint32 textureFormat, uint32 clutFormat, Nuanceur::CUintValue texPixel,
                                            Nuanceur::CArrayUintValue clutBuffer, Nuanceur::CIntValue texCsa)
{
	using namespace Nuanceur;

	assert(CGsPixelFormats::IsPsmIDTEX(textureFormat));

	bool idx8 = CGsPixelFormats::IsPsmIDTEX8(textureFormat) ? 1 : 0;
	auto clutIndex = CIntLvalue(b.CreateTemporaryInt());

	if(idx8)
	{
		clutIndex = ToInt(texPixel);
	}
	else
	{
		clutIndex = ToInt(texPixel) + texCsa;
	}

	switch(clutFormat)
	{
	default:
		assert(false);
	case CGSHandler::PSMCT32:
	case CGSHandler::PSMCT24:
	{
		auto clutIndexLo = (clutIndex & NewInt(b, 0xFF));
		auto clutIndexHi = (clutIndex & NewInt(b, 0xFF)) + NewInt(b, 0x100);
		auto clutPixelLo = Load(clutBuffer, clutIndexLo);
		auto clutPixelHi = Load(clutBuffer, clutIndexHi);
		auto clutPixel = clutPixelLo | (clutPixelHi << NewUint(b, 16));
		return CMemoryUtils::PSM32ToVec4(b, clutPixel);
	}
	case CGSHandler::PSMCT16:
	{
		auto clutPixel = Load(clutBuffer, clutIndex);
		return CMemoryUtils::PSM16ToVec4(b, clutPixel);
	}
	}
}

static Nuanceur::CFloat4Rvalue GetTextureColor(Nuanceur::CShaderBuilder& b, uint32 textureFormat, uint32 clutFormat,
                                               Nuanceur::CInt2Value texelPos, Nuanceur::CArrayUintValue memoryBuffer, Nuanceur::CArrayUintValue clutBuffer,
                                               Nuanceur::CImageUint2DValue texSwizzleTable, Nuanceur::CIntValue texBufAddress, Nuanceur::CIntValue texBufWidth,
                                               Nuanceur::CIntValue texCsa)
{
	using namespace Nuanceur;

	switch(textureFormat)
	{
	default:
		assert(false);
	case CGSHandler::PSMCT32:
	case CGSHandler::PSMZ32:
	{
		auto texAddress = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMCT32>(
		    b, texSwizzleTable, texBufAddress, texBufWidth, texelPos);
		auto texPixel = CMemoryUtils::Memory_Read32(b, memoryBuffer, texAddress);
		return CMemoryUtils::PSM32ToVec4(b, texPixel);
	}
	case CGSHandler::PSMCT24:
	case CGSHandler::PSMCT24_UNK:
	case CGSHandler::PSMZ24:
	{
		auto texAddress = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMCT32>(
		    b, texSwizzleTable, texBufAddress, texBufWidth, texelPos);
		auto texPixel = CMemoryUtils::Memory_Read24(b, memoryBuffer, texAddress);
		return CMemoryUtils::PSM32ToVec4(b, texPixel);
	}
	case CGSHandler::PSMCT16:
	case CGSHandler::PSMCT16S:
	case CGSHandler::PSMZ16:
	{
		auto texAddress = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMCT16>(
		    b, texSwizzleTable, texBufAddress, texBufWidth, texelPos);
		auto texPixel = CMemoryUtils::Memory_Read16(b, memoryBuffer, texAddress);
		return CMemoryUtils::PSM16ToVec4(b, texPixel);
	}
	case CGSHandler::PSMT8:
	{
		auto texAddress = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMT8>(
		    b, texSwizzleTable, texBufAddress, texBufWidth, texelPos);
		auto texPixel = CMemoryUtils::Memory_Read8(b, memoryBuffer, texAddress);
		return GetClutColor(b, textureFormat, clutFormat, texPixel, clutBuffer, texCsa);
	}
	case CGSHandler::PSMT4:
	{
		auto texAddress = CMemoryUtils::GetPixelAddress_PSMT4(
		    b, texSwizzleTable, texBufAddress, texBufWidth, texelPos);
		auto texPixel = CMemoryUtils::Memory_Read4(b, memoryBuffer, texAddress);
		return GetClutColor(b, textureFormat, clutFormat, texPixel, clutBuffer, texCsa);
	}
	case CGSHandler::PSMT8H:
	{
		auto texAddress = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMCT32>(
		    b, texSwizzleTable, texBufAddress, texBufWidth, texelPos);
		auto texPixel = CMemoryUtils::Memory_Read8(b, memoryBuffer, texAddress + NewInt(b, 3));
		return GetClutColor(b, textureFormat, clutFormat, texPixel, clutBuffer, texCsa);
	}
	case CGSHandler::PSMT4HL:
	{
		auto texAddress = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMCT32>(
		    b, texSwizzleTable, texBufAddress, texBufWidth, texelPos);
		auto texNibAddress = (texAddress + NewInt(b, 3)) * NewInt(b, 2);
		auto texPixel = CMemoryUtils::Memory_Read4(b, memoryBuffer, texNibAddress);
		return GetClutColor(b, textureFormat, clutFormat, texPixel, clutBuffer, texCsa);
	}
	case CGSHandler::PSMT4HH:
	{
		auto texAddress = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMCT32>(
		    b, texSwizzleTable, texBufAddress, texBufWidth, texelPos);
		auto texNibAddress = ((texAddress + NewInt(b, 3)) * NewInt(b, 2)) | NewInt(b, 1);
		auto texPixel = CMemoryUtils::Memory_Read4(b, memoryBuffer, texNibAddress);
		return GetClutColor(b, textureFormat, clutFormat, texPixel, clutBuffer, texCsa);
	}
	}
}

static void ExpandAlpha(Nuanceur::CShaderBuilder& b, uint32 textureFormat, uint32 clutFormat,
                        uint32 texBlackIsTransparent, Nuanceur::CFloat4Lvalue& textureColor,
                        Nuanceur::CFloatValue textureA0, Nuanceur::CFloatValue textureA1)
{
	using namespace Nuanceur;

	bool requiresExpansion = false;
	if(CGsPixelFormats::IsPsmIDTEX(textureFormat))
	{
		requiresExpansion =
		    (clutFormat == CGSHandler::PSMCT16) ||
		    (clutFormat == CGSHandler::PSMCT16S);
	}
	else
	{
		requiresExpansion =
		    (textureFormat == CGSHandler::PSMCT24) ||
		    (textureFormat == CGSHandler::PSMCT16) ||
		    (textureFormat == CGSHandler::PSMCT16S);
	}

	if(!requiresExpansion)
	{
		return;
	}

	auto alpha = Mix(textureA0, textureA1, textureColor->w());
	textureColor = NewFloat4(textureColor->xyz(), alpha);

	if(texBlackIsTransparent)
	{
		//Add rgb and check if it is zero (assume rgb is positive)
		//Set alpha to 0 if it is
		auto colorSum = textureColor->x() + textureColor->y() + textureColor->z();
		BeginIf(b, colorSum == NewFloat(b, 0));
		{
			textureColor = NewFloat4(textureColor->xyz(), NewFloat(b, 0));
		}
		EndIf(b);
	}
}

static Nuanceur::CFloat3Rvalue GetAlphaABD(Nuanceur::CShaderBuilder& b, uint32 alphaABD,
                                           Nuanceur::CFloat4Value srcColor, Nuanceur::CFloat4Value dstColor)
{
	switch(alphaABD)
	{
	default:
		assert(false);
	case CGSHandler::ALPHABLEND_ABD_CS:
		return srcColor->xyz();
	case CGSHandler::ALPHABLEND_ABD_CD:
		return dstColor->xyz();
	case CGSHandler::ALPHABLEND_ABD_ZERO:
		return NewFloat3(b, 0, 0, 0);
	}
}

static Nuanceur::CFloat3Rvalue GetAlphaC(Nuanceur::CShaderBuilder& b, uint32 alphaC,
                                         Nuanceur::CFloat4Value srcColor, Nuanceur::CFloat4Value dstColor, Nuanceur::CFloatValue alphaFix)
{
	switch(alphaC)
	{
	default:
		assert(false);
	case CGSHandler::ALPHABLEND_C_AS:
		return srcColor->www();
	case CGSHandler::ALPHABLEND_C_AD:
		return dstColor->www();
	case CGSHandler::ALPHABLEND_C_FIX:
		return alphaFix->xxx();
	}
}

static void DestinationAlphaTest(Nuanceur::CShaderBuilder& b, uint32 framebufferFormat,
                                 uint32 dstAlphaTestRef, Nuanceur::CUintValue dstPixel,
                                 Nuanceur::CBoolLvalue writeColor, Nuanceur::CBoolLvalue writeDepth)
{
	using namespace Nuanceur;

	auto alphaBit = CUintLvalue(b.CreateTemporaryUint());
	switch(framebufferFormat)
	{
	case CGSHandler::PSMCT32:
		alphaBit = dstPixel & NewUint(b, 0x80000000);
		break;
	case CGSHandler::PSMCT16:
	case CGSHandler::PSMCT16S:
		alphaBit = dstPixel & NewUint(b, 0x8000);
		break;
	default:
		assert(false);
		break;
	}

	auto dstAlphaTestResult = CBoolLvalue(b.CreateTemporaryBool());
	if(dstAlphaTestRef)
	{
		//Pixels with alpha bit set pass
		dstAlphaTestResult = (alphaBit != NewUint(b, 0));
	}
	else
	{
		dstAlphaTestResult = (alphaBit == NewUint(b, 0));
	}

	BeginIf(b, !dstAlphaTestResult);
	{
		writeColor = NewBool(b, false);
		writeDepth = NewBool(b, false);
	}
	EndIf(b);
}

static void WriteToFramebuffer(Nuanceur::CShaderBuilder& b, uint32 framebufferFormat,
                               Nuanceur::CArrayUintValue memoryBuffer, Nuanceur::CIntValue fbAddress,
                               Nuanceur::CUintValue fbWriteMask, Nuanceur::CUintValue dstPixel, Nuanceur::CFloat4Value dstColor)
{
	switch(framebufferFormat)
	{
	default:
		assert(false);
	case CGSHandler::PSMCT32:
	{
		dstPixel = (CMemoryUtils::Vec4ToPSM32(b, dstColor) & fbWriteMask) | (dstPixel & ~fbWriteMask);
		CMemoryUtils::Memory_Write32(b, memoryBuffer, fbAddress, dstPixel);
	}
	break;
	case CGSHandler::PSMCT24:
	case CGSHandler::PSMZ24:
	{
		dstPixel = (CMemoryUtils::Vec4ToPSM32(b, dstColor) & fbWriteMask) | (dstPixel & ~fbWriteMask);
		CMemoryUtils::Memory_Write24(b, memoryBuffer, fbAddress, dstPixel);
	}
	break;
	case CGSHandler::PSMCT16:
	case CGSHandler::PSMCT16S:
	{
		dstPixel = (CMemoryUtils::Vec4ToPSM16(b, dstColor) & fbWriteMask) | (dstPixel & ~fbWriteMask);
		CMemoryUtils::Memory_Write16(b, memoryBuffer, fbAddress, dstPixel);
	}
	break;
	}
}

static void WriteToDepthbuffer(Nuanceur::CShaderBuilder& b, uint32 depthbufferFormat,
                               Nuanceur::CArrayUintValue memoryBuffer, Nuanceur::CIntValue depthAddress, Nuanceur::CUintValue srcDepth)
{
	switch(depthbufferFormat)
	{
	case CGSHandler::PSMZ32:
	{
		CMemoryUtils::Memory_Write32(b, memoryBuffer, depthAddress, srcDepth);
	}
	break;
	case CGSHandler::PSMZ24:
	{
		auto dstDepth = srcDepth & NewUint(b, 0xFFFFFF);
		CMemoryUtils::Memory_Write24(b, memoryBuffer, depthAddress, dstDepth);
	}
	break;
	case CGSHandler::PSMZ16:
	case CGSHandler::PSMZ16S:
	{
		auto dstDepth = srcDepth & NewUint(b, 0xFFFF);
		CMemoryUtils::Memory_Write16(b, memoryBuffer, depthAddress, dstDepth);
	}
	break;
	default:
		assert(false);
		break;
	}
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
		auto outputColor = CFloat4Lvalue(b.CreateOutput(Nuanceur::SEMANTIC_SYSTEM_COLOR));
		auto outputDepth = CFloatLvalue(b.CreateOutput(Nuanceur::SEMANTIC_SYSTEM_DEPTH));

		auto memoryBuffer = CArrayUintValue(b.CreateUniformArrayUint("memoryBuffer", DESCRIPTOR_LOCATION_BUFFER_MEMORY, Nuanceur::SYMBOL_ATTRIBUTE_COHERENT));
		auto clutBuffer = CArrayUintValue(b.CreateUniformArrayUint("clutBuffer", DESCRIPTOR_LOCATION_IMAGE_CLUT));
		auto texSwizzleTable = CImageUint2DValue(b.CreateImage2DUint(DESCRIPTOR_LOCATION_IMAGE_SWIZZLETABLE_TEX));
		auto fbSwizzleTable = CImageUint2DValue(b.CreateImage2DUint(DESCRIPTOR_LOCATION_IMAGE_SWIZZLETABLE_FB));
		auto depthSwizzleTable = CImageUint2DValue(b.CreateImage2DUint(DESCRIPTOR_LOCATION_IMAGE_SWIZZLETABLE_DEPTH));
		auto subpassColorInput = CSubpassInputValue(b.CreateSubpassInput(DESCRIPTOR_LOCATION_IMAGE_INPUT_COLOR));
		auto subpassDepthInput = CSubpassInputValue(b.CreateSubpassInput(DESCRIPTOR_LOCATION_IMAGE_INPUT_DEPTH));

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

		auto texCsa = texParams1->x();
		auto texA0 = ToFloat(texParams1->y()) / NewFloat(b, 255.f);
		auto texA1 = ToFloat(texParams1->z()) / NewFloat(b, 255.f);

		auto clampMin = texParams2->xy();
		auto clampMax = texParams2->zw();

		auto fbWriteMask = ToUint(alphaFbParams->x());
		auto alphaFix = ToFloat(alphaFbParams->y()) / NewFloat(b, 255.f);
		auto alphaRef = ToUint(alphaFbParams->z());

		auto srcDepth = inputDepth->x();

		//TODO: Try vectorized shift
		//auto imageColor = ToUint(inputColor * NewFloat4(b, 255.f, 255.f, 255.f, 255.f));

		auto textureColor = CFloat4Lvalue(b.CreateVariableFloat("textureColor"));
		textureColor = NewFloat4(b, 1, 1, 1, 1);

		if(caps.hasTexture)
		{
			auto clampCoordinates =
			    [&](CInt2Value textureIuv) {
				    auto clampU = ClampTexCoord(b, caps.texClampU, textureIuv->x(), texSize->x(), clampMin->x(), clampMax->x());
				    auto clampV = ClampTexCoord(b, caps.texClampV, textureIuv->y(), texSize->y(), clampMin->y(), clampMax->y());
				    return NewInt2(clampU, clampV);
			    };

			auto getTextureColor =
			    [&](CInt2Value textureIuv, CFloat4Lvalue& textureColor) {
				    textureColor = GetTextureColor(b, caps.textureFormat, caps.clutFormat, textureIuv,
				                                   memoryBuffer, clutBuffer, texSwizzleTable, texBufAddress, texBufWidth, texCsa);
				    if(caps.textureHasAlpha)
				    {
					    ExpandAlpha(b, caps.textureFormat, caps.clutFormat, caps.textureBlackIsTransparent, textureColor, texA0, texA1);
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

		//---------------------------------------------------------------------------
		//Alpha Test

		bool canDiscardAlpha =
		    (caps.alphaTestFunction != CGSHandler::ALPHA_TEST_ALWAYS) &&
		    (caps.alphaTestFailAction == CGSHandler::ALPHA_TEST_FAIL_RGBONLY);
		auto alphaUint = ToUint(textureColor->w() * NewFloat(b, 255.f));
		auto alphaTestResult = CBoolLvalue(b.CreateTemporaryBool());
		switch(caps.alphaTestFunction)
		{
		default:
			assert(false);
		case CGSHandler::ALPHA_TEST_ALWAYS:
			alphaTestResult = NewBool(b, true);
			break;
		case CGSHandler::ALPHA_TEST_NEVER:
			alphaTestResult = NewBool(b, false);
			break;
		case CGSHandler::ALPHA_TEST_LESS:
			alphaTestResult = alphaUint < alphaRef;
			break;
		case CGSHandler::ALPHA_TEST_LEQUAL:
			alphaTestResult = alphaUint <= alphaRef;
			break;
		case CGSHandler::ALPHA_TEST_EQUAL:
			alphaTestResult = alphaUint == alphaRef;
			break;
		case CGSHandler::ALPHA_TEST_GEQUAL:
			alphaTestResult = alphaUint >= alphaRef;
			break;
		case CGSHandler::ALPHA_TEST_GREATER:
			alphaTestResult = alphaUint > alphaRef;
			break;
		case CGSHandler::ALPHA_TEST_NOTEQUAL:
			alphaTestResult = alphaUint != alphaRef;
			break;
		}

		BeginIf(b, !alphaTestResult);
		{
			switch(caps.alphaTestFailAction)
			{
			default:
				assert(false);
			case CGSHandler::ALPHA_TEST_FAIL_KEEP:
				writeColor = NewBool(b, false);
				writeDepth = NewBool(b, false);
				break;
			case CGSHandler::ALPHA_TEST_FAIL_FBONLY:
				writeDepth = NewBool(b, false);
				break;
			case CGSHandler::ALPHA_TEST_FAIL_ZBONLY:
				writeColor = NewBool(b, false);
				break;
			case CGSHandler::ALPHA_TEST_FAIL_RGBONLY:
				writeDepth = NewBool(b, false);
				writeAlpha = NewBool(b, false);
				break;
			}
		}
		EndIf(b);

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

		//BeginInvocationInterlock(b);

		auto dstPixel = CUintLvalue(b.CreateVariableUint("dstPixel"));
		auto dstColor = CFloat4Lvalue(b.CreateVariableFloat("dstColor"));
		auto dstAlpha = CFloat4Lvalue(b.CreateVariableFloat("dstAlpha"));
		auto dstDepth = CFloatLvalue(b.CreateVariableFloat("dstDepth"));
		
		auto finalColor = CFloat4Lvalue(b.CreateVariableFloat("finalColor"));
		auto finalDepth = CFloatLvalue(b.CreateVariableFloat("finalDepth"));

		dstPixel = NewUint(b, 0);
		dstColor = Load(subpassColorInput, NewInt2(b, 0, 0));
		dstDepth = Load(subpassDepthInput, NewInt2(b, 0, 0))->x();
		dstAlpha = NewFloat4(b, 1, 1, 1, 1);

		if(caps.hasDstAlphaTest)
		{
			DestinationAlphaTest(b, caps.framebufferFormat, caps.dstAlphaTestRef, dstPixel, writeColor, writeDepth);
		}

		auto depthTestResult = CBoolLvalue(b.CreateTemporaryBool());
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
			auto alphaA = GetAlphaABD(b, caps.alphaA, textureColor, dstColor);
			auto alphaB = GetAlphaABD(b, caps.alphaB, textureColor, dstColor);
			auto alphaC = GetAlphaC(b, caps.alphaC, textureColor, dstColor, alphaFix);
			auto alphaD = GetAlphaABD(b, caps.alphaD, textureColor, dstColor);

			auto blendedColor = ((alphaA - alphaB) * alphaC * NewFloat3(b, 2, 2, 2)) + alphaD;
			finalColor = NewFloat4(blendedColor, textureColor->w());
			finalColor = Clamp(finalColor, NewFloat4(b, 0, 0, 0, 0), NewFloat4(b, 1, 1, 1, 1));
		}
		else
		{
			finalColor = textureColor->xyzw();
		}

		finalDepth = srcDepth->x();

		BeginIf(b, !writeColor);
		{
			finalColor = dstColor->xyzw();
		}
		EndIf(b);

		BeginIf(b, !writeDepth);
		{
			finalDepth = dstDepth->x();
		}
		EndIf(b);

		if(canDiscardAlpha)
		{
			BeginIf(b, !writeAlpha);
			{
				dstColor = NewFloat4(dstColor->xyz(), dstAlpha->x());
			}
			EndIf(b);
		}

		//EndInvocationInterlock(b);

		outputColor = finalColor->xyzw();
		outputDepth = finalDepth->x();
	}

	Framework::CMemStream shaderStream;
	Nuanceur::CSpirvShaderGenerator::Generate(shaderStream, b, Nuanceur::CSpirvShaderGenerator::SHADER_TYPE_FRAGMENT);
	shaderStream.Seek(0, Framework::STREAM_SEEK_SET);
	return Framework::Vulkan::CShaderModule(m_context->device, shaderStream);
}

PIPELINE CDrawMobile::CreateLoadPipeline()
{
	PIPELINE loadPipeline;

	auto vertexShader = CreateLoadStoreVertexShader();
	auto fragmentShader = CreateLoadFragmentShader();

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
		pushConstantInfo.size = sizeof(LOAD_STORE_PIPELINE_PUSHCONSTANTS);

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
	VkPipelineColorBlendAttachmentState blendAttachment = {};
	blendAttachment.colorWriteMask = 0xf;

	auto colorBlendStateInfo = Framework::Vulkan::PipelineColorBlendStateCreateInfo();
	colorBlendStateInfo.attachmentCount = 1;
	colorBlendStateInfo.pAttachments = &blendAttachment;

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

PIPELINE CDrawMobile::CreateStorePipeline()
{
	PIPELINE storePipeline;

	auto vertexShader = CreateLoadStoreVertexShader();
	auto fragmentShader = CreateStoreFragmentShader();

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
		pushConstantInfo.size = sizeof(LOAD_STORE_PIPELINE_PUSHCONSTANTS);

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
	VkPipelineColorBlendAttachmentState blendAttachment = {};
	blendAttachment.colorWriteMask = 0xf;

	auto colorBlendStateInfo = Framework::Vulkan::PipelineColorBlendStateCreateInfo();
	colorBlendStateInfo.attachmentCount = 1;
	colorBlendStateInfo.pAttachments = &blendAttachment;

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

Framework::Vulkan::CShaderModule CDrawMobile::CreateLoadFragmentShader()
{
	LOADSTORE_CAPS caps;
	caps.framebufferFormat = CGSHandler::PSMCT32;
	caps.depthbufferFormat = CGSHandler::PSMZ32;

	using namespace Nuanceur;

	auto b = CShaderBuilder();

	{
		//Inputs
		auto inputPosition = CFloat4Lvalue(b.CreateInput(Nuanceur::SEMANTIC_SYSTEM_POSITION));

		//Outputs
		auto outputColor = CFloat4Lvalue(b.CreateOutput(Nuanceur::SEMANTIC_SYSTEM_COLOR));

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
		auto dstColor = CFloat4Lvalue(b.CreateVariableFloat("dstColor"));

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
			//TODO: Use real swizzle table
			depthAddress = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMZ16>(
			    b, depthSwizzleTable, depthBufAddress, depthBufWidth, screenPos);
			break;
		}

		switch(caps.framebufferFormat)
		{
		default:
			assert(false);
		case CGSHandler::PSMCT32:
		{
			dstPixel = CMemoryUtils::Memory_Read32(b, memoryBuffer, fbAddress);
			dstColor = CMemoryUtils::PSM32ToVec4(b, dstPixel);
		}
		break;
		case CGSHandler::PSMCT24:
		{
			dstPixel = CMemoryUtils::Memory_Read24(b, memoryBuffer, fbAddress);
			dstColor = CMemoryUtils::PSM32ToVec4(b, dstPixel);
		}
		break;
		case CGSHandler::PSMCT16:
		case CGSHandler::PSMCT16S:
		{
			dstPixel = CMemoryUtils::Memory_Read16(b, memoryBuffer, fbAddress);
			dstColor = CMemoryUtils::PSM16ToVec4(b, dstPixel);
		}
		break;
		}

		//if(needsDstDepth)
		if(false)
		{
			//dstDepth = GetDepth(b, caps.depthbufferFormat, depthAddress, memoryBuffer);
		}

		outputColor = dstColor->xyzw();
	}

	Framework::CMemStream shaderStream;
	Nuanceur::CSpirvShaderGenerator::Generate(shaderStream, b, Nuanceur::CSpirvShaderGenerator::SHADER_TYPE_FRAGMENT);
	shaderStream.Seek(0, Framework::STREAM_SEEK_SET);
	return Framework::Vulkan::CShaderModule(m_context->device, shaderStream);
}

Framework::Vulkan::CShaderModule CDrawMobile::CreateStoreFragmentShader()
{
	LOADSTORE_CAPS caps;
	caps.framebufferFormat = CGSHandler::PSMCT32;
	caps.depthbufferFormat = CGSHandler::PSMZ32;

	using namespace Nuanceur;

	auto b = CShaderBuilder();

	{
		//Inputs
		auto inputPosition = CFloat4Lvalue(b.CreateInput(Nuanceur::SEMANTIC_SYSTEM_POSITION));

		//Outputs
		auto outputColor = CFloat4Lvalue(b.CreateOutput(Nuanceur::SEMANTIC_SYSTEM_COLOR));

		auto memoryBuffer = CArrayUintValue(b.CreateUniformArrayUint("memoryBuffer", DESCRIPTOR_LOCATION_BUFFER_MEMORY, Nuanceur::SYMBOL_ATTRIBUTE_COHERENT));
		auto fbSwizzleTable = CImageUint2DValue(b.CreateImage2DUint(DESCRIPTOR_LOCATION_IMAGE_SWIZZLETABLE_FB));
		auto depthSwizzleTable = CImageUint2DValue(b.CreateImage2DUint(DESCRIPTOR_LOCATION_IMAGE_SWIZZLETABLE_DEPTH));
		auto subpassColorInput = CSubpassInputValue(b.CreateSubpassInput(DESCRIPTOR_LOCATION_IMAGE_INPUT_COLOR));
		auto subpassDepthInput = CSubpassInputValue(b.CreateSubpassInput(DESCRIPTOR_LOCATION_IMAGE_INPUT_DEPTH));

		//Push constants
		auto fbDepthParams = CInt4Lvalue(b.CreateUniformInt4("fbDepthParams", Nuanceur::UNIFORM_UNIT_PUSHCONSTANT));

		auto fbBufAddress = fbDepthParams->x();
		auto fbBufWidth = fbDepthParams->y();
		auto depthBufAddress = fbDepthParams->z();
		auto depthBufWidth = fbDepthParams->w();

		auto fbWriteMask = NewUint(b, 0xFFFFFFFF);
		auto dstPixel = NewUint(b, 0xFFFFFFFF);
		auto dstColor = Load(subpassColorInput, NewInt2(b, 0, 0));
		auto dstDepth = ToUint(Load(subpassDepthInput, NewInt2(b, 0, 0))->x() * NewFloat(b, DEPTH_MAX));

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
			//TODO: Use real swizzle table
			depthAddress = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMZ16>(
			    b, depthSwizzleTable, depthBufAddress, depthBufWidth, screenPos);
			break;
		}

		//Prevent writing out of bounds (seems to cause wierd issues
		//on Intel GPUs with games such as SNK vs. Capcom: SVC Chaos)
		fbAddress = fbAddress & NewInt(b, CGSHandler::RAMSIZE - 1);
		depthAddress = depthAddress & NewInt(b, CGSHandler::RAMSIZE - 1);

		WriteToFramebuffer(b, caps.framebufferFormat, memoryBuffer, fbAddress, fbWriteMask, dstPixel, dstColor);
		//WriteToDepthbuffer(b, caps.depthbufferFormat, memoryBuffer, depthAddress, dstDepth);

		outputColor = NewFloat4(b, 1, 1, 0, 1);
	}

	Framework::CMemStream shaderStream;
	Nuanceur::CSpirvShaderGenerator::Generate(shaderStream, b, Nuanceur::CSpirvShaderGenerator::SHADER_TYPE_FRAGMENT);
	shaderStream.Seek(0, Framework::STREAM_SEEK_SET);
	return Framework::Vulkan::CShaderModule(m_context->device, shaderStream);
}

void CDrawMobile::CreateDrawImages()
{
	m_drawColorImage = Framework::Vulkan::CImage(m_context->device, m_context->physicalDeviceMemoryProperties,
	                                             VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_FORMAT_R8G8B8A8_UNORM, DRAW_AREA_SIZE, DRAW_AREA_SIZE);

	m_drawColorImage.SetLayout(m_context->queue, m_context->commandBufferPool,
	                           VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

	m_drawColorImageView = m_drawColorImage.CreateImageView(VK_IMAGE_ASPECT_COLOR_BIT);

	m_drawDepthImage = Framework::Vulkan::CImage(m_context->device, m_context->physicalDeviceMemoryProperties,
	                                             VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_FORMAT_D32_SFLOAT, DRAW_AREA_SIZE, DRAW_AREA_SIZE);

	m_drawDepthImage.SetLayout(m_context->queue, m_context->commandBufferPool,
	                           VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
	
	m_drawDepthImageView = m_drawDepthImage.CreateImageView(VK_IMAGE_ASPECT_DEPTH_BIT);
}
