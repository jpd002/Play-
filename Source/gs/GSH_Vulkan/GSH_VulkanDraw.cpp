#include "GSH_VulkanDraw.h"
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
#define DESCRIPTOR_LOCATION_BUFFER_MEMORY_COPY 5

#define DRAW_AREA_SIZE 2048
#define MAX_VERTEX_COUNT 1024 * 512

#define DEPTH_MAX (4294967296.0f)

CDraw::CDraw(const ContextPtr& context, const FrameCommandBufferPtr& frameCommandBuffer)
    : m_context(context)
    , m_frameCommandBuffer(frameCommandBuffer)
    , m_pipelineCache(context->device)
{
	CreateRenderPass();
	CreateDrawImage();
	CreateFramebuffer();

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

CDraw::~CDraw()
{
	for(auto& frame : m_frames)
	{
		m_context->device.vkUnmapMemory(m_context->device, frame.vertexBuffer.GetMemory());
	}
	m_context->device.vkDestroyFramebuffer(m_context->device, m_framebuffer, nullptr);
	m_context->device.vkDestroyRenderPass(m_context->device, m_renderPass, nullptr);
	m_context->device.vkDestroyImageView(m_context->device, m_drawImageView, nullptr);
}

void CDraw::SetPipelineCaps(const PIPELINE_CAPS& caps)
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

void CDraw::SetFramebufferParams(uint32 addr, uint32 width, uint32 writeMask)
{
	bool changed =
	    (m_pushConstants.fbBufAddr != addr) ||
	    (m_pushConstants.fbBufWidth != width) ||
	    (m_pushConstants.fbWriteMask != writeMask);
	if(!changed) return;
	FlushVertices();
	m_pushConstants.fbBufAddr = addr;
	m_pushConstants.fbBufWidth = width;
	m_pushConstants.fbWriteMask = writeMask;
}

void CDraw::SetDepthbufferParams(uint32 addr, uint32 width)
{
	bool changed =
	    (m_pushConstants.depthBufAddr != addr) ||
	    (m_pushConstants.depthBufWidth != width);
	if(!changed) return;
	FlushVertices();
	m_pushConstants.depthBufAddr = addr;
	m_pushConstants.depthBufWidth = width;
}

void CDraw::SetTextureParams(uint32 bufAddr, uint32 bufWidth, uint32 width, uint32 height, uint32 csa)
{
	bool changed =
	    (m_pushConstants.texBufAddr != bufAddr) ||
	    (m_pushConstants.texBufWidth != bufWidth) ||
	    (m_pushConstants.texWidth != width) ||
	    (m_pushConstants.texHeight != height) ||
	    (m_pushConstants.texCsa != csa);
	if(!changed) return;
	FlushVertices();
	m_pushConstants.texBufAddr = bufAddr;
	m_pushConstants.texBufWidth = bufWidth;
	m_pushConstants.texWidth = width;
	m_pushConstants.texHeight = height;
	m_pushConstants.texCsa = csa;
}

void CDraw::SetClutBufferOffset(uint32 clutBufferOffset)
{
	bool changed = m_clutBufferOffset != clutBufferOffset;
	if(!changed) return;
	FlushVertices();
	m_clutBufferOffset = clutBufferOffset;
}

void CDraw::SetTextureAlphaParams(uint32 texA0, uint32 texA1)
{
	bool changed =
	    (m_pushConstants.texA0 != texA0) ||
	    (m_pushConstants.texA1 != texA1);
	if(!changed) return;
	FlushVertices();
	m_pushConstants.texA0 = texA0;
	m_pushConstants.texA1 = texA1;
}

void CDraw::SetTextureClampParams(uint32 clampMinU, uint32 clampMinV, uint32 clampMaxU, uint32 clampMaxV)
{
	bool changed =
	    (m_pushConstants.clampMin[0] != clampMinU) ||
	    (m_pushConstants.clampMin[1] != clampMinV) ||
	    (m_pushConstants.clampMax[0] != clampMaxU) ||
	    (m_pushConstants.clampMax[1] != clampMaxV);
	if(!changed) return;
	FlushVertices();
	m_pushConstants.clampMin[0] = clampMinU;
	m_pushConstants.clampMin[1] = clampMinV;
	m_pushConstants.clampMax[0] = clampMaxU;
	m_pushConstants.clampMax[1] = clampMaxV;
}

void CDraw::SetFogParams(float fogR, float fogG, float fogB)
{
	bool changed =
	    (m_pushConstants.fogColor[0] != fogR) ||
	    (m_pushConstants.fogColor[1] != fogG) ||
	    (m_pushConstants.fogColor[2] != fogB);
	if(!changed) return;
	FlushVertices();
	m_pushConstants.fogColor[0] = fogR;
	m_pushConstants.fogColor[1] = fogG;
	m_pushConstants.fogColor[2] = fogB;
	m_pushConstants.fogColor[3] = 0;
}

void CDraw::SetAlphaTestParams(uint32 alphaRef)
{
	bool changed = (m_pushConstants.alphaRef != alphaRef);
	if(!changed) return;
	FlushVertices();
	m_pushConstants.alphaRef = alphaRef;
}

void CDraw::SetAlphaBlendingParams(uint32 alphaFix)
{
	bool changed =
	    (m_pushConstants.alphaFix != alphaFix);
	if(!changed) return;
	FlushVertices();
	m_pushConstants.alphaFix = alphaFix;
}

void CDraw::SetScissor(uint32 scissorX, uint32 scissorY, uint32 scissorWidth, uint32 scissorHeight)
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

void CDraw::SetMemoryCopyParams(uint32 memoryCopyAddress, uint32 memoryCopySize)
{
	bool changed =
	    (memoryCopyAddress != m_memoryCopyAddress) ||
	    (memoryCopySize != m_memoryCopySize);
	if(!changed) return;
	FlushRenderPass();
	m_memoryCopyAddress = memoryCopyAddress;
	m_memoryCopySize = memoryCopySize;
}

void CDraw::AddVertices(const PRIM_VERTEX* vertexBeginPtr, const PRIM_VERTEX* vertexEndPtr)
{
	auto amount = vertexEndPtr - vertexBeginPtr;
	if((m_passVertexEnd + amount) > MAX_VERTEX_COUNT)
	{
		m_frameCommandBuffer->Flush();
		assert((m_passVertexEnd + amount) <= MAX_VERTEX_COUNT);
	}
	if(m_pipelineCaps.textureUseMemoryCopy)
	{
		//Check if sprite we are about to add overlaps with current region
		//Some games use tiny sprites to do full screen effects that requires
		//to keep a copy of RAM for texture sampling:
		//- Metal Gear Solid 3
		//- MK: Shaolin Monks
		//- Tales of Legendia
		const auto topLeftCorner = vertexBeginPtr;
		const auto bottomRightCorner = vertexBeginPtr + 5;
		CGsSpriteRect rect(topLeftCorner->x, topLeftCorner->y, bottomRightCorner->x, bottomRightCorner->y);
		if(m_memoryCopyRegion.Intersects(rect))
		{
			FlushRenderPass();
		}
		else
		{
			m_memoryCopyRegion.Insert(rect);
		}
	}
	auto& frame = m_frames[m_frameCommandBuffer->GetCurrentFrame()];
	memcpy(frame.vertexBufferPtr + m_passVertexEnd, vertexBeginPtr, amount * sizeof(PRIM_VERTEX));
	m_passVertexEnd += amount;
}

void CDraw::FlushVertices()
{
	uint32 vertexCount = m_passVertexEnd - m_passVertexStart;
	if(vertexCount == 0) return;

	auto& frame = m_frames[m_frameCommandBuffer->GetCurrentFrame()];
	auto commandBuffer = m_frameCommandBuffer->GetCommandBuffer();

	if(m_pipelineCaps.textureUseMemoryCopy)
	{
		assert(!m_renderPassBegun);

		//We need to keep a copy of the memory
		VkBufferCopy bufferCopy = {};
		bufferCopy.srcOffset = m_memoryCopyAddress;
		bufferCopy.dstOffset = m_memoryCopyAddress;
		bufferCopy.size = m_memoryCopySize;

		m_context->device.vkCmdCopyBuffer(commandBuffer, m_context->memoryBuffer, m_context->memoryBufferCopy, 1, &bufferCopy);

		m_memoryCopyRegion.Reset();
	}

	//Find pipeline and create it if we've never encountered it before
	auto drawPipeline = m_pipelineCache.TryGetPipeline(m_pipelineCaps);
	if(!drawPipeline)
	{
		drawPipeline = m_pipelineCache.RegisterPipeline(m_pipelineCaps, CreateDrawPipeline(m_pipelineCaps));
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

		m_renderPassBegun = true;
	}

	//Add a barrier to ensure reads/writes are complete before writing to GS memory
	//On Apple M1 GPU, this doesn't seem to be supported and needed, so we disable it
	//Not sure about Intel GPUs on macOS though
#ifndef __APPLE__
	{
		auto memoryBarrier = Framework::Vulkan::MemoryBarrier();
		memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

		m_context->device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		                                       VK_DEPENDENCY_BY_REGION_BIT, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
	}
#endif

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

void CDraw::FlushRenderPass()
{
	FlushVertices();
	if(m_renderPassBegun)
	{
		auto commandBuffer = m_frameCommandBuffer->GetCommandBuffer();
		m_context->device.vkCmdEndRenderPass(commandBuffer);
		m_renderPassBegun = false;
	}
}

void CDraw::PreFlushFrameCommandBuffer()
{
	FlushRenderPass();
}

void CDraw::PostFlushFrameCommandBuffer()
{
	m_passVertexStart = m_passVertexEnd = 0;
}

VkDescriptorSet CDraw::PrepareDescriptorSet(VkDescriptorSetLayout descriptorSetLayout, const DESCRIPTORSET_CAPS& caps)
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

		VkDescriptorBufferInfo descriptorMemoryCopyBufferInfo = {};
		descriptorMemoryCopyBufferInfo.buffer = m_context->memoryBufferCopy;
		descriptorMemoryCopyBufferInfo.range = VK_WHOLE_SIZE;

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
			writeSet.dstBinding = DESCRIPTOR_LOCATION_BUFFER_MEMORY_COPY;
			writeSet.descriptorCount = 1;
			writeSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			writeSet.pBufferInfo = &descriptorMemoryCopyBufferInfo;
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

		m_context->device.vkUpdateDescriptorSets(m_context->device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
	}

	m_descriptorSetCache.insert(std::make_pair(caps, descriptorSet));

	return descriptorSet;
}

void CDraw::CreateFramebuffer()
{
	assert(m_renderPass != VK_NULL_HANDLE);
	assert(m_framebuffer == VK_NULL_HANDLE);

	auto frameBufferCreateInfo = Framework::Vulkan::FramebufferCreateInfo();
	frameBufferCreateInfo.renderPass = m_renderPass;
	frameBufferCreateInfo.width = DRAW_AREA_SIZE;
	frameBufferCreateInfo.height = DRAW_AREA_SIZE;
	frameBufferCreateInfo.layers = 1;
	frameBufferCreateInfo.attachmentCount = 1;
	frameBufferCreateInfo.pAttachments = &m_drawImageView;

	auto result = m_context->device.vkCreateFramebuffer(m_context->device, &frameBufferCreateInfo, nullptr, &m_framebuffer);
	CHECKVULKANERROR(result);
}

void CDraw::CreateRenderPass()
{
	assert(m_renderPass == VK_NULL_HANDLE);

	auto result = VK_SUCCESS;

	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = VK_FORMAT_R8G8B8A8_UNORM;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorRef = {};
	colorRef.attachment = 0;
	colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.pColorAttachments = &colorRef;
	subpass.colorAttachmentCount = 1;

	VkSubpassDependency subpassDependency = {};
	subpassDependency.srcSubpass = 0;
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	subpassDependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	subpassDependency.dstSubpass = 0;
	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	subpassDependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	subpassDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	auto renderPassCreateInfo = Framework::Vulkan::RenderPassCreateInfo();
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &colorAttachment;
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &subpassDependency;

	result = m_context->device.vkCreateRenderPass(m_context->device, &renderPassCreateInfo, nullptr, &m_renderPass);
	CHECKVULKANERROR(result);
}

PIPELINE CDraw::CreateDrawPipeline(const PIPELINE_CAPS& caps)
{
	PIPELINE drawPipeline;

	auto vertexShader = CreateVertexShader(caps);
	auto fragmentShader = CreateFragmentShader(caps);

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
			setLayoutBinding.binding = DESCRIPTOR_LOCATION_BUFFER_MEMORY_COPY;
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
	case PIPELINE_PRIMITIVE_POINT:
		inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
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

Framework::Vulkan::CShaderModule CDraw::CreateVertexShader(const PIPELINE_CAPS& caps)
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
		auto outputPointSize = CFloatLvalue(b.CreateOutput(Nuanceur::SEMANTIC_SYSTEM_POINTSIZE));
		auto outputDepth = CFloat4Lvalue(b.CreateOutput(Nuanceur::SEMANTIC_TEXCOORD, 1));
		auto outputColor = CFloat4Lvalue(b.CreateOutput(Nuanceur::SEMANTIC_TEXCOORD, 2));
		auto outputTexCoord = CFloat4Lvalue(b.CreateOutput(Nuanceur::SEMANTIC_TEXCOORD, 3));
		auto outputFog = CFloat4Lvalue(b.CreateOutput(Nuanceur::SEMANTIC_TEXCOORD, 4));

		auto position = ((inputPosition->xy() + NewFloat2(b, 0.5f, 0.5f)) * NewFloat2(b, 2.f / DRAW_AREA_SIZE, 2.f / DRAW_AREA_SIZE) + NewFloat2(b, -1, -1));

		outputPosition = NewFloat4(position, NewFloat2(b, 0.f, 1.f));
		if(caps.primitiveType == PIPELINE_PRIMITIVE_POINT)
		{
			outputPointSize = NewFloat(b, 1.0f);
		}
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

static Nuanceur::CInt3Rvalue GetAlphaABD(Nuanceur::CShaderBuilder& b, uint32 alphaABD,
                                         Nuanceur::CInt4Value srcColor, Nuanceur::CInt4Value dstColor)
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
		return NewInt3(b, 0, 0, 0);
	}
}

static Nuanceur::CInt3Rvalue GetAlphaC(Nuanceur::CShaderBuilder& b, uint32 alphaC,
                                       Nuanceur::CInt4Value srcColor, Nuanceur::CInt4Value dstColor, Nuanceur::CIntValue alphaFix)
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

static void AlphaTest(Nuanceur::CShaderBuilder& b,
                      uint32 alphaTestFunction, uint32 alphaTestFailAction,
                      Nuanceur::CInt4Value srcIColor, Nuanceur::CIntValue alphaRef,
                      Nuanceur::CBoolLvalue writeColor, Nuanceur::CBoolLvalue writeDepth, Nuanceur::CBoolLvalue writeAlpha)
{
	using namespace Nuanceur;

	auto srcAlpha = srcIColor->w();
	auto alphaTestResult = CBoolLvalue(b.CreateVariableBool("alphaTestResult"));
	switch(alphaTestFunction)
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
		alphaTestResult = srcAlpha < alphaRef;
		break;
	case CGSHandler::ALPHA_TEST_LEQUAL:
		alphaTestResult = srcAlpha <= alphaRef;
		break;
	case CGSHandler::ALPHA_TEST_EQUAL:
		alphaTestResult = srcAlpha == alphaRef;
		break;
	case CGSHandler::ALPHA_TEST_GEQUAL:
		alphaTestResult = srcAlpha >= alphaRef;
		break;
	case CGSHandler::ALPHA_TEST_GREATER:
		alphaTestResult = srcAlpha > alphaRef;
		break;
	case CGSHandler::ALPHA_TEST_NOTEQUAL:
		alphaTestResult = srcAlpha != alphaRef;
		break;
	}

	BeginIf(b, !alphaTestResult);
	{
		switch(alphaTestFailAction)
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

	auto dstAlphaTestResult = CBoolLvalue(b.CreateVariableBool("dstAlphaTestResult"));
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
                               Nuanceur::CArrayUintValue memoryBuffer, Nuanceur::CIntValue fbAddress, Nuanceur::CUintValue srcPixel)
{
	switch(framebufferFormat)
	{
	default:
		assert(false);
		[[fallthrough]];
	case CGSHandler::PSMCT32:
	{
		CMemoryUtils::Memory_Write32(b, memoryBuffer, fbAddress, srcPixel);
	}
	break;
	case CGSHandler::PSMCT24:
	case CGSHandler::PSMZ24:
	{
		auto dstPixel = srcPixel & NewUint(b, 0xFFFFFF);
		CMemoryUtils::Memory_Write24(b, memoryBuffer, fbAddress, dstPixel);
	}
	break;
	case CGSHandler::PSMCT16:
	case CGSHandler::PSMCT16S:
	{
		auto dstPixel = srcPixel & NewUint(b, 0xFFFF);
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

Framework::Vulkan::CShaderModule CDraw::CreateFragmentShader(const PIPELINE_CAPS& caps)
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

		auto memoryBuffer = CArrayUintValue(b.CreateUniformArrayUint("memoryBuffer", DESCRIPTOR_LOCATION_BUFFER_MEMORY, Nuanceur::SYMBOL_ATTRIBUTE_COHERENT));
		auto memoryBufferCopy = CArrayUintValue(b.CreateUniformArrayUint("memoryBufferCopy", DESCRIPTOR_LOCATION_BUFFER_MEMORY_COPY));
		auto clutBuffer = CArrayUintValue(b.CreateUniformArrayUint("clutBuffer", DESCRIPTOR_LOCATION_IMAGE_CLUT));
		auto texSwizzleTable = CImageUint2DValue(b.CreateImage2DUint(DESCRIPTOR_LOCATION_IMAGE_SWIZZLETABLE_TEX));
		auto fbSwizzleTable = CImageUint2DValue(b.CreateImage2DUint(DESCRIPTOR_LOCATION_IMAGE_SWIZZLETABLE_FB));
		auto depthSwizzleTable = CImageUint2DValue(b.CreateImage2DUint(DESCRIPTOR_LOCATION_IMAGE_SWIZZLETABLE_DEPTH));

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
				    auto clampU = ClampTexCoord(b, caps.texClampU, textureIuv->x(), texSize->x(), clampMin->x(), clampMax->x());
				    auto clampV = ClampTexCoord(b, caps.texClampV, textureIuv->y(), texSize->y(), clampMin->y(), clampMax->y());
				    return NewInt2(clampU, clampV);
			    };

			auto getTextureColor =
			    [&](CInt2Value textureIuv, CFloat4Lvalue& textureColor) {
				    if(caps.textureUseMemoryCopy)
				    {
					    textureColor = GetTextureColor(b, caps.textureFormat, caps.clutFormat, textureIuv,
					                                   memoryBufferCopy, clutBuffer, texSwizzleTable, texBufAddress, texBufWidth, texCsa);
				    }
				    else
				    {
					    textureColor = GetTextureColor(b, caps.textureFormat, caps.clutFormat, textureIuv,
					                                   memoryBuffer, clutBuffer, texSwizzleTable, texBufAddress, texBufWidth, texCsa);
				    }
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

		if(caps.hasFog)
		{
			auto fogMixColor = Mix(textureColor->xyz(), fogColor->xyz(), inputFog->xxx());
			textureColor = NewFloat4(fogMixColor, textureColor->w());
		}

		auto writeColor = CBoolLvalue(b.CreateVariableBool("writeColor"));
		auto writeDepth = CBoolLvalue(b.CreateVariableBool("writeDepth"));
		auto writeAlpha = CBoolLvalue(b.CreateVariableBool("writeAlpha"));

		writeColor = NewBool(b, true);
		writeDepth = NewBool(b, true);
		writeAlpha = NewBool(b, true);

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

		auto fbAddress = CIntLvalue(b.CreateVariableInt("fbAddress"));
		auto depthAddress = CIntLvalue(b.CreateVariableInt("depthAddress"));

		switch(caps.framebufferFormat)
		{
		default:
			assert(false);
			[[fallthrough]];
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
			[[fallthrough]];
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

		auto srcIColor = CInt4Lvalue(b.CreateVariableInt("srcIColor"));
		srcIColor = ToInt(textureColor->xyzw() * NewFloat4(b, 255.f, 255.f, 255.f, 255.f));

		AlphaTest(b, caps.alphaTestFunction, caps.alphaTestFailAction, srcIColor, alphaRef,
		          writeColor, writeDepth, writeAlpha);

		BeginInvocationInterlock(b);

		auto dstPixel = CUintLvalue(b.CreateVariableUint("dstPixel"));
		auto dstDepth = CUintLvalue(b.CreateVariableUint("dstDepth"));
		auto dstIColor = CInt4Lvalue(b.CreateVariableInt("dstIColor"));

		auto finalPixel = CUintLvalue(b.CreateVariableUint("finalPixel"));
		auto finalIColor = CInt4Lvalue(b.CreateVariableInt("finalIColor"));

		bool canDiscardAlpha =
		    (caps.alphaTestFunction != CGSHandler::ALPHA_TEST_ALWAYS) &&
		    (caps.alphaTestFailAction == CGSHandler::ALPHA_TEST_FAIL_RGBONLY);
		bool needsDstColor = (caps.hasAlphaBlending != 0) || (caps.maskColor != 0) || canDiscardAlpha || (caps.hasDstAlphaTest != 0);
		if(needsDstColor)
		{
			switch(caps.framebufferFormat)
			{
			default:
				assert(false);
				[[fallthrough]];
			case CGSHandler::PSMCT32:
			{
				dstPixel = CMemoryUtils::Memory_Read32(b, memoryBuffer, fbAddress);
				dstIColor = CMemoryUtils::PSM32ToIVec4(b, dstPixel);
			}
			break;
			case CGSHandler::PSMCT24:
			{
				dstPixel = CMemoryUtils::Memory_Read24(b, memoryBuffer, fbAddress);
				dstIColor = CMemoryUtils::PSM32ToIVec4(b, dstPixel);
			}
			break;
			case CGSHandler::PSMCT16:
			case CGSHandler::PSMCT16S:
			{
				dstPixel = CMemoryUtils::Memory_Read16(b, memoryBuffer, fbAddress);
				dstIColor = CMemoryUtils::PSM16ToIVec4(b, dstPixel);
			}
			break;
			}
		}
		else
		{
			dstPixel = NewUint(b, 0);
			dstIColor = NewInt4(b, 0, 0, 0, 0);
		}

		if(caps.hasDstAlphaTest)
		{
			DestinationAlphaTest(b, caps.framebufferFormat, caps.dstAlphaTestRef, dstPixel, writeColor, writeDepth);
		}

		bool needsDstDepth = (caps.depthTestFunction == CGSHandler::DEPTH_TEST_GEQUAL) ||
		                     (caps.depthTestFunction == CGSHandler::DEPTH_TEST_GREATER);
		if(needsDstDepth)
		{
			dstDepth = GetDepth(b, caps.depthbufferFormat, depthAddress, memoryBuffer);
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
			auto alphaA = GetAlphaABD(b, caps.alphaA, srcIColor, dstIColor);
			auto alphaB = GetAlphaABD(b, caps.alphaB, srcIColor, dstIColor);
			auto alphaC = GetAlphaC(b, caps.alphaC, srcIColor, dstIColor, alphaFix);
			auto alphaD = GetAlphaABD(b, caps.alphaD, srcIColor, dstIColor);

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
				finalIColor = NewInt4(finalIColor->xyz(), dstIColor->x());
			}
			EndIf(b);
		}

		BeginIf(b, writeColor);
		{
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

			WriteToFramebuffer(b, caps.framebufferFormat, memoryBuffer, fbAddress, finalPixel);
		}
		EndIf(b);

		if(caps.writeDepth)
		{
			BeginIf(b, writeDepth);
			{
				WriteToDepthbuffer(b, caps.depthbufferFormat, memoryBuffer, depthAddress, srcDepth);
			}
			EndIf(b);
		}

		EndInvocationInterlock(b);

		outputColor = ToFloat(finalIColor) / NewFloat4(b, 255.f, 255.f, 255.f, 255.f);
	}

	Framework::CMemStream shaderStream;
	Nuanceur::CSpirvShaderGenerator::Generate(shaderStream, b, Nuanceur::CSpirvShaderGenerator::SHADER_TYPE_FRAGMENT);
	shaderStream.Seek(0, Framework::STREAM_SEEK_SET);
	return Framework::Vulkan::CShaderModule(m_context->device, shaderStream);
}

void CDraw::CreateDrawImage()
{
	//This image is needed for MoltenVK/Metal which seem to discard pixels
	//that don't write to any color attachment

	m_drawImage = Framework::Vulkan::CImage(m_context->device, m_context->physicalDeviceMemoryProperties,
	                                        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R8G8B8A8_UNORM, DRAW_AREA_SIZE, DRAW_AREA_SIZE);

	m_drawImage.SetLayout(m_context->queue, m_context->commandBufferPool,
	                      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

	m_drawImageView = m_drawImage.CreateImageView();
}
