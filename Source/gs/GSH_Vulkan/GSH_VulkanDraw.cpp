#include "GSH_VulkanDraw.h"
#include "MemStream.h"
#include "vulkan/StructDefs.h"
#include "vulkan/Utils.h"
#include "nuanceur/Builder.h"
#include "nuanceur/generators/SpirvShaderGenerator.h"

using namespace GSH_Vulkan;

#define DESCRIPTOR_LOCATION_IMAGE_MEMORY 0
#define DESCRIPTOR_LOCATION_UNIFORM_FRAMEBUFFER 1

static void MakeLinearZOrtho(float* matrix, float left, float right, float bottom, float top)
{
	matrix[0] = 2.0f / (right - left);
	matrix[1] = 0;
	matrix[2] = 0;
	matrix[3] = 0;

	matrix[4] = 0;
	matrix[5] = 2.0f / (top - bottom);
	matrix[6] = 0;
	matrix[7] = 0;

	matrix[8] = 0;
	matrix[9] = 0;
	matrix[10] = 1;
	matrix[11] = 0;

	matrix[12] = -(right + left) / (right - left);
	matrix[13] = -(top + bottom) / (top - bottom);
	matrix[14] = 0;
	matrix[15] = 1;
}

#define DRAW_AREA_SIZE 1024
#define MAX_VERTEX_COUNT 1024

CDraw::CDraw(const ContextPtr& context)
	: m_context(context)
{
	CreateRenderPass();
	CreateDrawImage();
	CreateFramebuffer();
	CreateVertexShader();
	CreateFragmentShader();
	CreateDrawPipeline();

	m_vertexBuffer = Framework::Vulkan::CBuffer(
		m_context->device, m_context->physicalDeviceMemoryProperties,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, sizeof(PRIM_VERTEX) * MAX_VERTEX_COUNT);
	
	auto result = m_context->device.vkMapMemory(m_context->device, m_vertexBuffer.GetMemory(), 
		0, VK_WHOLE_SIZE, 0, reinterpret_cast<void**>(&m_vertexBufferPtr));
	CHECKVULKANERROR(result);

	m_frameBufferUniform = Framework::Vulkan::CBuffer(
		m_context->device, m_context->physicalDeviceMemoryProperties,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, sizeof(FRAMEBUFFER));

	MakeLinearZOrtho(m_vertexShaderConstants.projMatrix, 0, DRAW_AREA_SIZE, 0, DRAW_AREA_SIZE);
}

CDraw::~CDraw()
{
	m_context->device.vkUnmapMemory(m_context->device, m_vertexBuffer.GetMemory());
	m_context->device.vkDestroyPipeline(m_context->device, m_drawPipeline, nullptr);
	m_context->device.vkDestroyPipelineLayout(m_context->device, m_drawPipelineLayout, nullptr);
	m_context->device.vkDestroyDescriptorSetLayout(m_context->device, m_drawDescriptorSetLayout, nullptr);
	m_context->device.vkDestroyFramebuffer(m_context->device, m_framebuffer, nullptr);
	m_context->device.vkDestroyRenderPass(m_context->device, m_renderPass, nullptr);
	m_context->device.vkDestroyImageView(m_context->device, m_drawImageView, nullptr);
	m_context->device.vkDestroyImage(m_context->device, m_drawImage, nullptr);
	m_context->device.vkFreeMemory(m_context->device, m_drawImageMemoryHandle, nullptr);
}

void CDraw::SetFramebuffer(uint32 ptr, uint32 width)
{
	m_fbBuffer.bufAddress = ptr;
	m_fbBuffer.bufWidth = width;
}

void CDraw::AddVertices(const PRIM_VERTEX* vertexBeginPtr, const PRIM_VERTEX* vertexEndPtr)
{
	auto amount = vertexEndPtr - vertexBeginPtr;
	if((m_passVertexEnd + amount) > MAX_VERTEX_COUNT)
	{
		FlushCommands();
	}
	memcpy(m_vertexBufferPtr + m_passVertexEnd, vertexBeginPtr, amount * sizeof(PRIM_VERTEX));
	m_passVertexEnd += amount;
}

void CDraw::FlushVertices()
{
	uint32 vertexCount = m_passVertexEnd - m_passVertexStart;
	if(vertexCount == 0) return;

	if(m_commandBuffer == VK_NULL_HANDLE)
	{
		StartRecording();
	}

	//-------------------
	//Update Framebuffer Info Uniform

	m_context->device.vkCmdUpdateBuffer(m_commandBuffer, m_frameBufferUniform, 0, sizeof(FRAMEBUFFER), &m_fbBuffer);

	auto renderPassBeginInfo = Framework::Vulkan::RenderPassBeginInfo();
	renderPassBeginInfo.renderPass               = m_renderPass;
	renderPassBeginInfo.renderArea.extent.width  = DRAW_AREA_SIZE;
	renderPassBeginInfo.renderArea.extent.height = DRAW_AREA_SIZE;
	renderPassBeginInfo.framebuffer              = m_framebuffer;
	m_context->device.vkCmdBeginRenderPass(m_commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	auto descriptorSet = PrepareDescriptorSet();

	m_context->device.vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_drawPipelineLayout,
		0, 1, &descriptorSet, 0, nullptr);

	m_context->device.vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_drawPipeline);

	VkDeviceSize vertexBufferOffset = (m_passVertexStart * sizeof(PRIM_VERTEX));
	VkBuffer vertexBuffer = m_vertexBuffer;
	m_context->device.vkCmdBindVertexBuffers(m_commandBuffer, 0, 1, &vertexBuffer, &vertexBufferOffset);

	m_context->device.vkCmdPushConstants(m_commandBuffer, m_drawPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VERTEX_SHADER_CONSTANTS), &m_vertexShaderConstants);

	assert((vertexCount % 3) == 0);
	m_context->device.vkCmdDraw(m_commandBuffer, vertexCount, 1, 0, 0);

	m_context->device.vkCmdEndRenderPass(m_commandBuffer);

	m_passVertexStart = m_passVertexEnd;
}

void CDraw::FlushCommands()
{
	FlushVertices();

	if(m_commandBuffer == VK_NULL_HANDLE) return;

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
	m_passVertexStart = m_passVertexEnd = 0;
}

VkDescriptorSet CDraw::PrepareDescriptorSet()
{
	auto result = VK_SUCCESS;
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

		VkDescriptorBufferInfo descriptorUniformInfo = {};
		descriptorUniformInfo.buffer = m_frameBufferUniform;
		descriptorUniformInfo.range = VK_WHOLE_SIZE;

		std::vector<VkWriteDescriptorSet> writes;

		{
			auto writeSet = Framework::Vulkan::WriteDescriptorSet();
			writeSet.dstSet          = descriptorSet;
			writeSet.dstBinding      = DESCRIPTOR_LOCATION_IMAGE_MEMORY;
			writeSet.descriptorCount = 1;
			writeSet.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			writeSet.pImageInfo      = &descriptorImageInfo;
			writes.push_back(writeSet);
		}

		{
			auto writeSet = Framework::Vulkan::WriteDescriptorSet();
			writeSet.dstSet          = descriptorSet;
			writeSet.dstBinding      = DESCRIPTOR_LOCATION_UNIFORM_FRAMEBUFFER;
			writeSet.descriptorCount = 1;
			writeSet.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeSet.pBufferInfo      = &descriptorUniformInfo;
			writes.push_back(writeSet);
		}

		m_context->device.vkUpdateDescriptorSets(m_context->device, writes.size(), writes.data(), 0, nullptr);
	}

	return descriptorSet;
}

void CDraw::StartRecording()
{
	assert(m_commandBuffer == VK_NULL_HANDLE);

	auto result = VK_SUCCESS;
	m_commandBuffer = m_context->commandBufferPool.AllocateBuffer();

	auto commandBufferBeginInfo = Framework::Vulkan::CommandBufferBeginInfo();
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	result = m_context->device.vkBeginCommandBuffer(m_commandBuffer, &commandBufferBeginInfo);
	CHECKVULKANERROR(result);

	{
		VkViewport viewport = {};
		viewport.width    = DRAW_AREA_SIZE;
		viewport.height   = DRAW_AREA_SIZE;
		viewport.maxDepth = 1.0f;
		m_context->device.vkCmdSetViewport(m_commandBuffer, 0, 1, &viewport);
		
		VkRect2D scissor = {};
		scissor.extent.width  = DRAW_AREA_SIZE;
		scissor.extent.height = DRAW_AREA_SIZE;
		m_context->device.vkCmdSetScissor(m_commandBuffer, 0, 1, &scissor);
	}
}

void CDraw::CreateFramebuffer()
{
	assert(m_renderPass != VK_NULL_HANDLE);
	assert(m_framebuffer == VK_NULL_HANDLE);

	auto frameBufferCreateInfo = Framework::Vulkan::FramebufferCreateInfo();
	frameBufferCreateInfo.renderPass      = m_renderPass;
	frameBufferCreateInfo.width           = DRAW_AREA_SIZE;
	frameBufferCreateInfo.height          = DRAW_AREA_SIZE;
	frameBufferCreateInfo.layers          = 1;
	frameBufferCreateInfo.attachmentCount = 1;
	frameBufferCreateInfo.pAttachments    = &m_drawImageView;
		
	auto result = m_context->device.vkCreateFramebuffer(m_context->device, &frameBufferCreateInfo, nullptr, &m_framebuffer);
	CHECKVULKANERROR(result);
}

void CDraw::CreateRenderPass()
{
	assert(m_renderPass == VK_NULL_HANDLE);
	
	auto result = VK_SUCCESS;

	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format         = VK_FORMAT_R8G8B8A8_UNORM;
	colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorRef = {};
	colorRef.attachment = 0;
	colorRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.pColorAttachments    = &colorRef;
	subpass.colorAttachmentCount = 1;

	auto renderPassCreateInfo = Framework::Vulkan::RenderPassCreateInfo();
	renderPassCreateInfo.subpassCount    = 1;
	renderPassCreateInfo.pSubpasses      = &subpass;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments    = &colorAttachment;

	result = m_context->device.vkCreateRenderPass(m_context->device, &renderPassCreateInfo, nullptr, &m_renderPass);
	CHECKVULKANERROR(result);
}

void CDraw::CreateDrawPipeline()
{
	assert(!m_vertexShader.IsEmpty());
	assert(!m_fragmentShader.IsEmpty());
	assert(m_drawPipeline == VK_NULL_HANDLE);
	assert(m_drawPipelineLayout == VK_NULL_HANDLE);
	assert(m_drawDescriptorSetLayout == VK_NULL_HANDLE);

	auto result = VK_SUCCESS;

	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;

		{
			VkDescriptorSetLayoutBinding setLayoutBinding = {};
			setLayoutBinding.binding         = DESCRIPTOR_LOCATION_IMAGE_MEMORY;
			setLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			setLayoutBinding.descriptorCount = 1;
			setLayoutBinding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
			setLayoutBindings.push_back(setLayoutBinding);
		}

		{
			VkDescriptorSetLayoutBinding setLayoutBinding = {};
			setLayoutBinding.binding         = DESCRIPTOR_LOCATION_UNIFORM_FRAMEBUFFER;
			setLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			setLayoutBinding.descriptorCount = 1;
			setLayoutBinding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
			setLayoutBindings.push_back(setLayoutBinding);
		}

		auto setLayoutCreateInfo = Framework::Vulkan::DescriptorSetLayoutCreateInfo();
		setLayoutCreateInfo.bindingCount = setLayoutBindings.size();
		setLayoutCreateInfo.pBindings    = setLayoutBindings.data();

		result = m_context->device.vkCreateDescriptorSetLayout(m_context->device, &setLayoutCreateInfo, nullptr, &m_drawDescriptorSetLayout);
		CHECKVULKANERROR(result);
	}

	{
		VkPushConstantRange pushConstantInfo = {};
		pushConstantInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushConstantInfo.offset     = 0;
		pushConstantInfo.size       = sizeof(VERTEX_SHADER_CONSTANTS);

		auto pipelineLayoutCreateInfo = Framework::Vulkan::PipelineLayoutCreateInfo();
		pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pipelineLayoutCreateInfo.pPushConstantRanges    = &pushConstantInfo;
		pipelineLayoutCreateInfo.setLayoutCount         = 1;
		pipelineLayoutCreateInfo.pSetLayouts            = &m_drawDescriptorSetLayout;

		result = m_context->device.vkCreatePipelineLayout(m_context->device, &pipelineLayoutCreateInfo, nullptr, &m_drawPipelineLayout);
		CHECKVULKANERROR(result);
	}

	auto inputAssemblyInfo = Framework::Vulkan::PipelineInputAssemblyStateCreateInfo();
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	
	std::vector<VkVertexInputAttributeDescription> vertexAttributes;

	{
		VkVertexInputAttributeDescription vertexAttributeDesc = {};
		vertexAttributeDesc.format = VK_FORMAT_R32G32_SFLOAT;
		vertexAttributeDesc.offset = offsetof(PRIM_VERTEX, x);
		vertexAttributeDesc.location = 0;
		vertexAttributes.push_back(vertexAttributeDesc);
	}

	{
		VkVertexInputAttributeDescription vertexAttributeDesc = {};
		vertexAttributeDesc.format = VK_FORMAT_R32_UINT;
		vertexAttributeDesc.offset = offsetof(PRIM_VERTEX, z);
		vertexAttributeDesc.location = 1;
		vertexAttributes.push_back(vertexAttributeDesc);
	}

	{
		VkVertexInputAttributeDescription vertexAttributeDesc = {};
		vertexAttributeDesc.format = VK_FORMAT_R8G8B8A8_UNORM;
		vertexAttributeDesc.offset = offsetof(PRIM_VERTEX, color);
		vertexAttributeDesc.location = 2;
		vertexAttributes.push_back(vertexAttributeDesc);
	}

	VkVertexInputBindingDescription binding = {};
	binding.stride    = sizeof(PRIM_VERTEX);
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

void CDraw::CreateVertexShader()
{
	using namespace Nuanceur;
	
	auto b = CShaderBuilder();
	
	{
		//Vertex Inputs
		auto inputPosition = CFloat4Lvalue(b.CreateInput(Nuanceur::SEMANTIC_POSITION));
		auto inputColor = CFloat4Lvalue(b.CreateInput(Nuanceur::SEMANTIC_TEXCOORD, 1));

		//Outputs
		auto outputPosition = CFloat4Lvalue(b.CreateOutput(Nuanceur::SEMANTIC_SYSTEM_POSITION));
		auto outputColor = CFloat4Lvalue(b.CreateOutput(Nuanceur::SEMANTIC_TEXCOORD, 1));

		//Constants
		auto projMatrix = CMatrix44Value(b.CreateUniformMatrix("g_projMatrix", Nuanceur::UNIFORM_UNIT_PUSHCONSTANT));

		outputPosition = projMatrix * NewFloat4(inputPosition->xyz(), NewFloat(b, 1.0f));
		outputColor = inputColor->xyzw();
	}
	
	Framework::CMemStream shaderStream;
	Nuanceur::CSpirvShaderGenerator::Generate(shaderStream, b, Nuanceur::CSpirvShaderGenerator::SHADER_TYPE_VERTEX);
	shaderStream.Seek(0, Framework::STREAM_SEEK_SET);
	m_vertexShader = Framework::Vulkan::CShaderModule(m_context->device, shaderStream);
}

void CDraw::CreateFragmentShader()
{
	using namespace Nuanceur;
	
	auto b = CShaderBuilder();
	
	{
		//Inputs
		auto inputPosition = CFloat4Lvalue(b.CreateInput(Nuanceur::SEMANTIC_SYSTEM_POSITION));
		auto inputColor = CFloat4Lvalue(b.CreateInput(Nuanceur::SEMANTIC_TEXCOORD, 1));

		//Outputs
		auto outputColor = CFloat4Lvalue(b.CreateOutput(Nuanceur::SEMANTIC_SYSTEM_COLOR));

		auto memoryImage = CImageUint2DValue(b.CreateImage2DUint(DESCRIPTOR_LOCATION_IMAGE_MEMORY));

		auto fbParams0 = CInt4Lvalue(b.CreateUniformInt4("fbParams0", DESCRIPTOR_LOCATION_UNIFORM_FRAMEBUFFER));
		auto fbAddress = fbParams0->x();
		auto fbWidth   = fbParams0->y();

		//TODO: Try vectorized shift
		//auto imageColor = ToUint(inputColor * NewFloat4(b, 255.f, 255.f, 255.f, 255.f));
		
		auto imageColorR = ToUint(inputColor->x() * NewFloat(b, 255.f)) << NewUint(b,  0);
		auto imageColorG = ToUint(inputColor->y() * NewFloat(b, 255.f)) << NewUint(b,  8);
		auto imageColorB = ToUint(inputColor->z() * NewFloat(b, 255.f)) << NewUint(b, 16);
		auto imageColorA = ToUint(inputColor->w() * NewFloat(b, 255.f)) << NewUint(b, 24);

		//auto imageColor = CUint4Lvalue(b.CreateConstantUint(0, 0, 0, 0));
		auto imageColor = imageColorR | imageColorG | imageColorB | imageColorA;

		static int32 c_texelSize = 4;
		static int32 c_memorySize = 1024;

		auto screenPos = ToInt(inputPosition->xy());
		auto address = fbAddress + (screenPos->y() * fbWidth * NewInt(b, c_texelSize)) + (screenPos->x() * NewInt(b, c_texelSize));

		auto wordAddress = address / NewInt(b, 4);
		auto position = NewInt2(wordAddress % NewInt(b, c_memorySize), wordAddress / NewInt(b, c_memorySize));

		BeginInvocationInterlock(b);
		Store(memoryImage, position, NewUint4(imageColor, NewUint3(b, 0, 0, 0)));
		EndInvocationInterlock(b);

		outputColor = NewFloat4(b, 0, 0, 1, 0);
	}
	
	Framework::CMemStream shaderStream;
	Nuanceur::CSpirvShaderGenerator::Generate(shaderStream, b, Nuanceur::CSpirvShaderGenerator::SHADER_TYPE_FRAGMENT);
	shaderStream.Seek(0, Framework::STREAM_SEEK_SET);
	m_fragmentShader = Framework::Vulkan::CShaderModule(m_context->device, shaderStream);
}

void CDraw::CreateDrawImage()
{
	//This image is needed for MoltenVK/Metal which seem to discard pixels
	//that don't write to any color attachment

	{
		auto imageCreateInfo = Framework::Vulkan::ImageCreateInfo();
		imageCreateInfo.imageType     = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format        = VK_FORMAT_R8G8B8A8_UNORM;
		imageCreateInfo.extent.width  = DRAW_AREA_SIZE;
		imageCreateInfo.extent.height = DRAW_AREA_SIZE;
		imageCreateInfo.extent.depth  = 1;
		imageCreateInfo.mipLevels     = 1;
		imageCreateInfo.arrayLayers   = 1;
		imageCreateInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.usage         = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		auto result = m_context->device.vkCreateImage(m_context->device, &imageCreateInfo, nullptr, &m_drawImage);
		CHECKVULKANERROR(result);
	}

	{
		VkMemoryRequirements memoryRequirements = {};
		m_context->device.vkGetImageMemoryRequirements(m_context->device, m_drawImage, &memoryRequirements);

		auto memoryAllocateInfo = Framework::Vulkan::MemoryAllocateInfo();
		memoryAllocateInfo.allocationSize  = memoryRequirements.size;
		memoryAllocateInfo.memoryTypeIndex = Framework::Vulkan::GetMemoryTypeIndex(
			m_context->physicalDeviceMemoryProperties, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		auto result = m_context->device.vkAllocateMemory(m_context->device, &memoryAllocateInfo, nullptr, &m_drawImageMemoryHandle);
		CHECKVULKANERROR(result);
	}
	
	m_context->device.vkBindImageMemory(m_context->device, m_drawImage, m_drawImageMemoryHandle, 0);

	{
		auto imageViewCreateInfo = Framework::Vulkan::ImageViewCreateInfo();
		imageViewCreateInfo.image    = m_drawImage;
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format   = VK_FORMAT_R8G8B8A8_UNORM;
		imageViewCreateInfo.components = 
		{
			VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, 
			VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A 
		};
		imageViewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		
		auto result = m_context->device.vkCreateImageView(m_context->device, &imageViewCreateInfo, nullptr, &m_drawImageView);
		CHECKVULKANERROR(result);
	}
}
