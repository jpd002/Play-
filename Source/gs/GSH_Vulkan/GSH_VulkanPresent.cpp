#include "GSH_VulkanPresent.h"
#include "MemStream.h"
#include "vulkan/StructDefs.h"
#include "vulkan/Utils.h"
#include "nuanceur/Builder.h"
#include "nuanceur/generators/SpirvShaderGenerator.h"

using namespace GSH_Vulkan;

#define DESCRIPTOR_LOCATION_MEMORY 0

//Module responsible for presenting frame buffer to surface

// clang-format off
const CPresent::PRESENT_VERTEX CPresent::g_vertexBufferContents[3] =
{
	//Pos           UV
	{ -1.0f, -1.0f, 0.0f,  1.0f, },
	{ -1.0f,  3.0f, 0.0f, -1.0f, },
	{  3.0f, -1.0f, 2.0f,  1.0f, },
};
// clang-format on

CPresent::CPresent(const ContextPtr& context)
	: m_context(context)
{
	//Create the semaphore that will be used to prevent submit from rendering before getting the image
	{
		auto semaphoreCreateInfo = Framework::Vulkan::SemaphoreCreateInfo();
		auto result = m_context->device.vkCreateSemaphore(m_context->device, &semaphoreCreateInfo, nullptr, &m_imageAcquireSemaphore);
		CHECKVULKANERROR(result);
	}
	
	{
		auto semaphoreCreateInfo = Framework::Vulkan::SemaphoreCreateInfo();
		auto result = m_context->device.vkCreateSemaphore(m_context->device, &semaphoreCreateInfo, nullptr, &m_renderCompleteSemaphore);
		CHECKVULKANERROR(result);
	}

	CreateSwapChain();
	CreateSwapChainImageViews();
	CreateRenderPass();
	CreateSwapChainFramebuffers();
	CreateVertexShader();
	CreateFragmentShader();
	CreateVertexBuffer();
	CreateDrawPipeline();
}

CPresent::~CPresent()
{
	m_vertexShader.Reset();
	m_fragmentShader.Reset();
	m_context->device.vkDestroyBuffer(m_context->device, m_vertexBuffer, nullptr);
	m_context->device.vkFreeMemory(m_context->device, m_vertexBufferMemory, nullptr);
	m_context->device.vkDestroyPipeline(m_context->device, m_drawPipeline, nullptr);
	m_context->device.vkDestroyPipelineLayout(m_context->device, m_drawPipelineLayout, nullptr);
	m_context->device.vkDestroyDescriptorSetLayout(m_context->device, m_drawDescriptorSetLayout, nullptr);
	m_context->device.vkDestroyRenderPass(m_context->device, m_renderPass, nullptr);
	for(auto swapChainFramebuffer : m_swapChainFramebuffers)
	{
		m_context->device.vkDestroyFramebuffer(m_context->device, swapChainFramebuffer, nullptr);
	}
	for(auto swapChainImageView : m_swapChainImageViews)
	{
		m_context->device.vkDestroyImageView(m_context->device, swapChainImageView, nullptr);
	}
	m_context->device.vkDestroySwapchainKHR(m_context->device, m_swapChain, nullptr);
	m_context->device.vkDestroySemaphore(m_context->device, m_imageAcquireSemaphore, nullptr);
	m_context->device.vkDestroySemaphore(m_context->device, m_renderCompleteSemaphore, nullptr);
}

void CPresent::DoPresent()
{
	auto result = VK_SUCCESS;

	uint32_t imageIndex = 0;
	result = m_context->device.vkAcquireNextImageKHR(m_context->device, m_swapChain, UINT64_MAX, m_imageAcquireSemaphore, VK_NULL_HANDLE, &imageIndex);
	CHECKVULKANERROR(result);

	UpdateBackbuffer(imageIndex);

	//Queue present
	{
		auto presentInfo = Framework::Vulkan::PresentInfoKHR();
		presentInfo.swapchainCount     = 1;
		presentInfo.pSwapchains        = &m_swapChain;
		presentInfo.pImageIndices      = &imageIndex;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores    = &m_renderCompleteSemaphore;
		result = m_context->device.vkQueuePresentKHR(m_context->queue, &presentInfo);
		CHECKVULKANERROR(result);
	}
	
	//result = m_context->device.vkQueueWaitIdle(m_queue);
	//CHECKVULKANERROR(result);
}

void CPresent::UpdateBackbuffer(uint32 imageIndex)
{
	auto result = VK_SUCCESS;

	auto swapChainImage = m_swapChainImages[imageIndex];
	auto framebuffer = m_swapChainFramebuffers[imageIndex];

	auto commandBuffer = m_context->commandBufferPool.AllocateBuffer();

	auto commandBufferBeginInfo = Framework::Vulkan::CommandBufferBeginInfo();
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	result = m_context->device.vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
	CHECKVULKANERROR(result);

	//Transition image from present to color attachment
	{
		auto imageMemoryBarrier = Framework::Vulkan::ImageMemoryBarrier();
		imageMemoryBarrier.image               = swapChainImage;
		imageMemoryBarrier.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
		//imageMemoryBarrier.srcAccessMask       = VK_ACCESS_MEMORY_READ_BIT;
		imageMemoryBarrier.newLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		imageMemoryBarrier.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.subresourceRange    = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		
		m_context->device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
	}

	VkClearValue clearValue;
	clearValue.color = { { 0.5f, 0.5f, 0.5f, 1.0f } };

	//Begin render pass
	auto renderPassBeginInfo = Framework::Vulkan::RenderPassBeginInfo();
	renderPassBeginInfo.renderPass               = m_renderPass;
	renderPassBeginInfo.renderArea.extent        = m_context->surfaceExtents;
	renderPassBeginInfo.clearValueCount          = 1;
	renderPassBeginInfo.pClearValues             = &clearValue;
	renderPassBeginInfo.framebuffer              = framebuffer;

	m_context->device.vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	{
		VkViewport viewport = {};
		viewport.width    = m_context->surfaceExtents.width;
		viewport.height   = m_context->surfaceExtents.height;
		viewport.maxDepth = 1.0f;
		m_context->device.vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		
		VkRect2D scissor = {};
		scissor.extent  = m_context->surfaceExtents;
		m_context->device.vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
	
	//Allocate descriptor set
	{
		auto setAllocateInfo = Framework::Vulkan::DescriptorSetAllocateInfo();
		setAllocateInfo.descriptorPool     = m_context->descriptorPool;
		setAllocateInfo.descriptorSetCount = 1;
		setAllocateInfo.pSetLayouts        = &m_drawDescriptorSetLayout;

		result = m_context->device.vkAllocateDescriptorSets(m_context->device, &setAllocateInfo, &descriptorSet);
		CHECKVULKANERROR(result);
	}

	//Update descriptor set
	{
		VkDescriptorImageInfo descriptorImageInfo = {};
		descriptorImageInfo.imageView = m_context->memoryImageView;
		descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		auto writeSet = Framework::Vulkan::WriteDescriptorSet();
		writeSet.dstSet          = descriptorSet;
		writeSet.dstBinding      = DESCRIPTOR_LOCATION_MEMORY;
		writeSet.descriptorCount = 1;
		writeSet.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		writeSet.pImageInfo      = &descriptorImageInfo;

		m_context->device.vkUpdateDescriptorSets(m_context->device, 1, &writeSet, 0, nullptr);
	}

	m_context->device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_drawPipelineLayout,
		0, 1, &descriptorSet, 0, nullptr);

	m_context->device.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_drawPipeline);

	VkDeviceSize vertexBufferOffset = 0;
	m_context->device.vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_vertexBuffer, &vertexBufferOffset);

	m_context->device.vkCmdDraw(commandBuffer, 3, 1, 0, 0);

	m_context->device.vkCmdEndRenderPass(commandBuffer);

	//Transition image from attachment to present
	{
		auto imageMemoryBarrier = Framework::Vulkan::ImageMemoryBarrier();
		imageMemoryBarrier.image               = swapChainImage;
		imageMemoryBarrier.oldLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		imageMemoryBarrier.srcAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		imageMemoryBarrier.newLayout           = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		imageMemoryBarrier.dstAccessMask       = VK_ACCESS_MEMORY_READ_BIT;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.subresourceRange    = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		
		m_context->device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
	}

	m_context->device.vkEndCommandBuffer(commandBuffer);

	//Submit command buffer
	{
		VkPipelineStageFlags pipelineStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		auto submitInfo = Framework::Vulkan::SubmitInfo();
		submitInfo.waitSemaphoreCount   = 1;
		submitInfo.pWaitSemaphores      = &m_imageAcquireSemaphore;
		submitInfo.pWaitDstStageMask    = &pipelineStageFlags;
		submitInfo.commandBufferCount   = 1;
		submitInfo.pCommandBuffers      = &commandBuffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores    = &m_renderCompleteSemaphore;
		result = m_context->device.vkQueueSubmit(m_context->queue, 1, &submitInfo, VK_NULL_HANDLE);
		CHECKVULKANERROR(result);
	}
}

void CPresent::CreateSwapChain()
{
	assert(!m_context->device.IsEmpty());
	assert(m_swapChain == VK_NULL_HANDLE);
	assert(m_swapChainImages.empty());

	auto result = VK_SUCCESS;
	
	auto swapChainCreateInfo = Framework::Vulkan::SwapchainCreateInfoKHR();
	swapChainCreateInfo.surface               = m_context->surface;
	swapChainCreateInfo.minImageCount         = 3; //Recommended by nVidia in UsingtheVulkanAPI_20160216.pdf
	swapChainCreateInfo.imageFormat           = m_context->surfaceFormat.format;
	swapChainCreateInfo.imageColorSpace       = m_context->surfaceFormat.colorSpace;
	swapChainCreateInfo.imageExtent           = m_context->surfaceExtents;
	swapChainCreateInfo.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapChainCreateInfo.preTransform          = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swapChainCreateInfo.imageArrayLayers      = 1;
	swapChainCreateInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
	swapChainCreateInfo.queueFamilyIndexCount = 0;
	swapChainCreateInfo.pQueueFamilyIndices   = nullptr;
	swapChainCreateInfo.presentMode           = VK_PRESENT_MODE_FIFO_KHR;
	swapChainCreateInfo.clipped               = VK_TRUE;
	swapChainCreateInfo.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	result = m_context->device.vkCreateSwapchainKHR(m_context->device, &swapChainCreateInfo, nullptr, &m_swapChain);
	CHECKVULKANERROR(result);
	
	uint32_t imageCount = 0;
	result = m_context->device.vkGetSwapchainImagesKHR(m_context->device, m_swapChain, &imageCount, nullptr);
	CHECKVULKANERROR(result);
	
	m_swapChainImages.resize(imageCount);
	result = m_context->device.vkGetSwapchainImagesKHR(m_context->device, m_swapChain, &imageCount, m_swapChainImages.data());
	CHECKVULKANERROR(result);
}

void CPresent::CreateSwapChainImageViews()
{
	assert(!m_context->device.IsEmpty());
	assert(m_swapChainImageViews.empty());
	
	for(const auto& image : m_swapChainImages)
	{
		auto imageViewCreateInfo = Framework::Vulkan::ImageViewCreateInfo();
		imageViewCreateInfo.format     = m_context->surfaceFormat.format;
		imageViewCreateInfo.viewType   = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.image      = image;
		imageViewCreateInfo.components = 
		{ 
			VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
			VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A
		};
		imageViewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		
		VkImageView imageView = VK_NULL_HANDLE;
		auto result = m_context->device.vkCreateImageView(m_context->device, &imageViewCreateInfo, nullptr, &imageView);
		CHECKVULKANERROR(result);
		m_swapChainImageViews.push_back(imageView);
	}
}

void CPresent::CreateSwapChainFramebuffers()
{
	assert(!m_context->device.IsEmpty());
	assert(!m_swapChainImageViews.empty());
	
	for(const auto& imageView : m_swapChainImageViews)
	{
		auto frameBufferCreateInfo = Framework::Vulkan::FramebufferCreateInfo();
		frameBufferCreateInfo.renderPass      = m_renderPass;
		frameBufferCreateInfo.attachmentCount = 1;
		frameBufferCreateInfo.pAttachments    = &imageView;
		frameBufferCreateInfo.width           = m_context->surfaceExtents.width;
		frameBufferCreateInfo.height          = m_context->surfaceExtents.height;
		frameBufferCreateInfo.layers          = 1;
		
		VkFramebuffer framebuffer = VK_NULL_HANDLE;
		auto result = m_context->device.vkCreateFramebuffer(m_context->device, &frameBufferCreateInfo, nullptr, &framebuffer);
		CHECKVULKANERROR(result);
		m_swapChainFramebuffers.push_back(framebuffer);
	}
}

void CPresent::CreateRenderPass()
{
	assert(m_renderPass == VK_NULL_HANDLE);
	
	auto result = VK_SUCCESS;
	
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format         = m_context->surfaceFormat.format;
	colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	
	VkAttachmentReference colorRef = {};
	colorRef.attachment = 0;
	colorRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount    = 1;
	subpass.pColorAttachments       = &colorRef;
	
	auto renderPassCreateInfo = Framework::Vulkan::RenderPassCreateInfo();
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments    = &colorAttachment;
	renderPassCreateInfo.subpassCount    = 1;
	renderPassCreateInfo.pSubpasses      = &subpass;

	result = m_context->device.vkCreateRenderPass(m_context->device, &renderPassCreateInfo, nullptr, &m_renderPass);
	CHECKVULKANERROR(result);
}

void CPresent::CreateDrawPipeline()
{
	assert(!m_vertexShader.IsEmpty());
	assert(!m_fragmentShader.IsEmpty());
	assert(m_drawPipeline == VK_NULL_HANDLE);
	assert(m_drawPipelineLayout == VK_NULL_HANDLE);
	assert(m_drawDescriptorSetLayout == VK_NULL_HANDLE);

	auto result = VK_SUCCESS;

	{
		VkDescriptorSetLayoutBinding setLayoutBinding = {};
		setLayoutBinding.binding         = 0;
		setLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		setLayoutBinding.descriptorCount = 1;
		setLayoutBinding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

		auto setLayoutCreateInfo = Framework::Vulkan::DescriptorSetLayoutCreateInfo();
		setLayoutCreateInfo.bindingCount = 1;
		setLayoutCreateInfo.pBindings    = &setLayoutBinding;
		
		result = m_context->device.vkCreateDescriptorSetLayout(m_context->device, &setLayoutCreateInfo, nullptr, &m_drawDescriptorSetLayout);
		CHECKVULKANERROR(result);
	}
	
	{
		auto pipelineLayoutCreateInfo = Framework::Vulkan::PipelineLayoutCreateInfo();
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pSetLayouts    = &m_drawDescriptorSetLayout;

		result = m_context->device.vkCreatePipelineLayout(m_context->device, &pipelineLayoutCreateInfo, nullptr, &m_drawPipelineLayout);
		CHECKVULKANERROR(result);
	}

	auto inputAssemblyInfo = Framework::Vulkan::PipelineInputAssemblyStateCreateInfo();
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	
	std::vector<VkVertexInputAttributeDescription> vertexAttributes;

	{
		VkVertexInputAttributeDescription positionVertexAttributeDesc = {};
		positionVertexAttributeDesc.format = VK_FORMAT_R32G32_SFLOAT;
		positionVertexAttributeDesc.offset = offsetof(PRESENT_VERTEX, x);
		positionVertexAttributeDesc.location = 0;
		vertexAttributes.push_back(positionVertexAttributeDesc);
	}

	{
		VkVertexInputAttributeDescription texCoordVertexAttributeDesc = {};
		texCoordVertexAttributeDesc.format = VK_FORMAT_R32G32_SFLOAT;
		texCoordVertexAttributeDesc.offset = offsetof(PRESENT_VERTEX, u);
		texCoordVertexAttributeDesc.location = 1;
		vertexAttributes.push_back(texCoordVertexAttributeDesc);
	}

	VkVertexInputBindingDescription binding = {};
	binding.stride    = sizeof(PRESENT_VERTEX);
	binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	
	auto vertexInputInfo = Framework::Vulkan::PipelineVertexInputStateCreateInfo();
	vertexInputInfo.vertexBindingDescriptionCount   = 1;
	vertexInputInfo.pVertexBindingDescriptions      = &binding;
	vertexInputInfo.vertexAttributeDescriptionCount = vertexAttributes.size();
	vertexInputInfo.pVertexAttributeDescriptions    = vertexAttributes.data();

	auto rasterStateInfo = Framework::Vulkan::PipelineRasterizationStateCreateInfo();
	rasterStateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterStateInfo.cullMode    = VK_CULL_MODE_NONE;
	rasterStateInfo.lineWidth   = 1.0f;
	
	// Our attachment will write to all color channels, but no blending is enabled.
	VkPipelineColorBlendAttachmentState blendAttachment = {};
	blendAttachment.colorWriteMask = 0xf;
	
	auto colorBlendStateInfo = Framework::Vulkan::PipelineColorBlendStateCreateInfo();
	colorBlendStateInfo.attachmentCount = 1;
	colorBlendStateInfo.pAttachments    = &blendAttachment;
	
	auto viewportStateInfo = Framework::Vulkan::PipelineViewportStateCreateInfo();
	viewportStateInfo.viewportCount = 1;
	viewportStateInfo.scissorCount  = 1;
	
	auto depthStencilStateInfo = Framework::Vulkan::PipelineDepthStencilStateCreateInfo();
	
	auto multisampleStateInfo = Framework::Vulkan::PipelineMultisampleStateCreateInfo();
	multisampleStateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	static const VkDynamicState dynamicStates[] = 
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};
	auto dynamicStateInfo = Framework::Vulkan::PipelineDynamicStateCreateInfo();
	dynamicStateInfo.pDynamicStates    = dynamicStates;
	dynamicStateInfo.dynamicStateCount = sizeof(dynamicStates) / sizeof(dynamicStates[0]);
	
	VkPipelineShaderStageCreateInfo shaderStages[2] =
	{
		{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO },
		{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO },
	};
	
	shaderStages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = m_vertexShader;
	shaderStages[0].pName  = "main";
	shaderStages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = m_fragmentShader;
	shaderStages[1].pName  = "main";

	auto pipelineCreateInfo = Framework::Vulkan::GraphicsPipelineCreateInfo();
	pipelineCreateInfo.stageCount          = 2;
	pipelineCreateInfo.pStages             = shaderStages;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyInfo;
	pipelineCreateInfo.pVertexInputState   = &vertexInputInfo;
	pipelineCreateInfo.pRasterizationState = &rasterStateInfo;
	pipelineCreateInfo.pColorBlendState    = &colorBlendStateInfo;
	pipelineCreateInfo.pViewportState      = &viewportStateInfo;
	pipelineCreateInfo.pDepthStencilState  = &depthStencilStateInfo;
	pipelineCreateInfo.pMultisampleState   = &multisampleStateInfo;
	pipelineCreateInfo.pDynamicState       = &dynamicStateInfo;
	pipelineCreateInfo.renderPass          = m_renderPass;
	pipelineCreateInfo.layout              = m_drawPipelineLayout;
	
	result = m_context->device.vkCreateGraphicsPipelines(m_context->device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_drawPipeline);
	CHECKVULKANERROR(result);
}

void CPresent::CreateVertexShader()
{
	using namespace Nuanceur;
	
	auto b = CShaderBuilder();
	
	{
		auto inputPosition = CFloat4Lvalue(b.CreateInput(Nuanceur::SEMANTIC_POSITION));
		auto outputPosition = CFloat4Lvalue(b.CreateOutput(Nuanceur::SEMANTIC_SYSTEM_POSITION));
		outputPosition = NewFloat4(inputPosition->xyz(), NewFloat(b, 1.0f));
	}
	
	Framework::CMemStream shaderStream;
	Nuanceur::CSpirvShaderGenerator::Generate(shaderStream, b, Nuanceur::CSpirvShaderGenerator::SHADER_TYPE_VERTEX);
	shaderStream.Seek(0, Framework::STREAM_SEEK_SET);
	m_vertexShader = Framework::Vulkan::CShaderModule(m_context->device, shaderStream);
}

void CPresent::CreateFragmentShader()
{
	using namespace Nuanceur;
	
	auto b = CShaderBuilder();
	
	{
		auto inputPosition = CFloat4Lvalue(b.CreateInput(Nuanceur::SEMANTIC_SYSTEM_POSITION));
		auto outputColor = CFloat4Lvalue(b.CreateOutput(Nuanceur::SEMANTIC_SYSTEM_COLOR));
		auto memoryImage = CImageUint2DValue(b.CreateImage2DUint(DESCRIPTOR_LOCATION_MEMORY));

		auto imageColor = Load(memoryImage, ToInt(inputPosition->xy()));
		outputColor = ToFloat(imageColor) / NewFloat4(b, 255.f, 255.f, 255.f, 255.f);
	}
	
	Framework::CMemStream shaderStream;
	//Framework::CStdStream shaderStream("frag.spv", "wb");
	Nuanceur::CSpirvShaderGenerator::Generate(shaderStream, b, Nuanceur::CSpirvShaderGenerator::SHADER_TYPE_FRAGMENT);
	shaderStream.Seek(0, Framework::STREAM_SEEK_SET);
	m_fragmentShader = Framework::Vulkan::CShaderModule(m_context->device, shaderStream);
}

void CPresent::CreateVertexBuffer()
{
	auto result = VK_SUCCESS;

	auto bufferCreateInfo = Framework::Vulkan::BufferCreateInfo();
	bufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufferCreateInfo.size  = sizeof(g_vertexBufferContents);
	result = m_context->device.vkCreateBuffer(m_context->device, &bufferCreateInfo, nullptr, &m_vertexBuffer);
	CHECKVULKANERROR(result);
	
	VkMemoryRequirements memoryRequirements = {};
	m_context->device.vkGetBufferMemoryRequirements(m_context->device, m_vertexBuffer, &memoryRequirements);

	auto memoryAllocateInfo = Framework::Vulkan::MemoryAllocateInfo();
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = Framework::Vulkan::GetMemoryTypeIndex(m_context->physicalDeviceMemoryProperties, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	assert(memoryAllocateInfo.memoryTypeIndex != Framework::Vulkan::VULKAN_MEMORY_TYPE_INVALID);

	result = m_context->device.vkAllocateMemory(m_context->device, &memoryAllocateInfo, nullptr, &m_vertexBufferMemory);
	CHECKVULKANERROR(result);
	
	result = m_context->device.vkBindBufferMemory(m_context->device, m_vertexBuffer, m_vertexBufferMemory, 0);
	CHECKVULKANERROR(result);

	{
		void* bufferMemoryData = nullptr;
		result = m_context->device.vkMapMemory(m_context->device, m_vertexBufferMemory, 0, VK_WHOLE_SIZE, 0, &bufferMemoryData);
		CHECKVULKANERROR(result);
		memcpy(bufferMemoryData, g_vertexBufferContents, sizeof(g_vertexBufferContents));
		m_context->device.vkUnmapMemory(m_context->device, m_vertexBufferMemory);
	}
}
