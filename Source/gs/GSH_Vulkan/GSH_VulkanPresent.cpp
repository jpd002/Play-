#include "GSH_Vulkan.h"
#include "MemStream.h"
#include "vulkan/StructDefs.h"
#include "vulkan/Utils.h"
#include "nuanceur/Builder.h"
#include "nuanceur/generators/SpirvShaderGenerator.h"

// clang-format off
const CGSH_Vulkan::PRESENT_VERTEX CGSH_Vulkan::g_presentVertexBufferContents[3] =
{
	//Pos         UV
	{ -1.0f, -1.0f, 0.0f,  1.0f, },
	{ -1.0f,  3.0f, 0.0f, -1.0f, },
	{  3.0f, -1.0f, 2.0f,  1.0f, },
};
// clang-format on

void CGSH_Vulkan::InitializePresent(VkFormat surfaceFormat)
{
	CreatePresentRenderPass(surfaceFormat);
	CreatePresentVertexShader();
	CreatePresentFragmentShader();
	CreatePresentVertexBuffer();
	CreatePresentDrawPipeline();
}

void CGSH_Vulkan::DestroyPresent()
{
	m_presentVertexShader.Reset();
	m_presentFragmentShader.Reset();
	m_device.vkDestroyBuffer(m_device, m_presentVertexBuffer, nullptr);
	m_device.vkFreeMemory(m_device, m_presentVertexBufferMemory, nullptr);
	m_device.vkDestroyPipeline(m_device, m_presentDrawPipeline, nullptr);
	m_device.vkDestroyPipelineLayout(m_device, m_presentDrawPipelineLayout, nullptr);
	m_device.vkDestroyRenderPass(m_device, m_presentRenderPass, nullptr);
}

void CGSH_Vulkan::CreatePresentRenderPass(VkFormat colorFormat)
{
	assert(!m_device.IsEmpty());
	assert(m_presentRenderPass == VK_NULL_HANDLE);
	
	auto result = VK_SUCCESS;
	
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format         = colorFormat;
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

	result = m_device.vkCreateRenderPass(m_device, &renderPassCreateInfo, nullptr, &m_presentRenderPass);
	CHECKVULKANERROR(result);
}

void CGSH_Vulkan::CreatePresentDrawPipeline()
{
	//assert(!m_vertexShaderModule.IsEmpty());
	//assert(!m_fragmentShaderModule.IsEmpty());
	assert(m_presentDrawPipeline == VK_NULL_HANDLE);
	assert(m_presentDrawPipelineLayout == VK_NULL_HANDLE);

	auto result = VK_SUCCESS;

	auto pipelineLayoutCreateInfo = Framework::Vulkan::PipelineLayoutCreateInfo();
	result = m_device.vkCreatePipelineLayout(m_device, &pipelineLayoutCreateInfo, nullptr, &m_presentDrawPipelineLayout);
	CHECKVULKANERROR(result);
	
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
	shaderStages[0].module = m_presentVertexShader;
	shaderStages[0].pName  = "main";
	shaderStages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = m_presentFragmentShader;
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
	pipelineCreateInfo.renderPass          = m_presentRenderPass;
	pipelineCreateInfo.layout              = m_presentDrawPipelineLayout;
	
	result = m_device.vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_presentDrawPipeline);
	CHECKVULKANERROR(result);
}

void CGSH_Vulkan::CreatePresentVertexShader()
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
	m_presentVertexShader = Framework::Vulkan::CShaderModule(m_device, shaderStream);
}

void CGSH_Vulkan::CreatePresentFragmentShader()
{
	using namespace Nuanceur;
	
	auto b = CShaderBuilder();
	
	{
		auto outputColor = CFloat4Lvalue(b.CreateOutput(Nuanceur::SEMANTIC_SYSTEM_COLOR));
		outputColor = NewFloat4(b, 1, 0, 0, 1);
	}
	
	Framework::CMemStream shaderStream;
	Nuanceur::CSpirvShaderGenerator::Generate(shaderStream, b, Nuanceur::CSpirvShaderGenerator::SHADER_TYPE_FRAGMENT);
	shaderStream.Seek(0, Framework::STREAM_SEEK_SET);
	m_presentFragmentShader = Framework::Vulkan::CShaderModule(m_device, shaderStream);
}

void CGSH_Vulkan::CreatePresentVertexBuffer()
{
	auto result = VK_SUCCESS;

	auto bufferCreateInfo = Framework::Vulkan::BufferCreateInfo();
	bufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufferCreateInfo.size  = sizeof(g_presentVertexBufferContents);
	result = m_device.vkCreateBuffer(m_device, &bufferCreateInfo, nullptr, &m_presentVertexBuffer);
	CHECKVULKANERROR(result);
	
	VkMemoryRequirements memoryRequirements = {};
	m_device.vkGetBufferMemoryRequirements(m_device, m_presentVertexBuffer, &memoryRequirements);

	auto memoryAllocateInfo = Framework::Vulkan::MemoryAllocateInfo();
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = Framework::Vulkan::GetMemoryTypeIndex(m_physicalDeviceMemoryProperties, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	assert(memoryAllocateInfo.memoryTypeIndex != Framework::Vulkan::VULKAN_MEMORY_TYPE_INVALID);

	result = m_device.vkAllocateMemory(m_device, &memoryAllocateInfo, nullptr, &m_presentVertexBufferMemory);
	CHECKVULKANERROR(result);
	
	result = m_device.vkBindBufferMemory(m_device, m_presentVertexBuffer, m_presentVertexBufferMemory, 0);
	CHECKVULKANERROR(result);

	{
		void* bufferMemoryData = nullptr;
		result = m_device.vkMapMemory(m_device, m_presentVertexBufferMemory, 0, VK_WHOLE_SIZE, 0, &bufferMemoryData);
		CHECKVULKANERROR(result);
		memcpy(bufferMemoryData, g_presentVertexBufferContents, sizeof(g_presentVertexBufferContents));
		m_device.vkUnmapMemory(m_device, m_presentVertexBufferMemory);
	}
}
