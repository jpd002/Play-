#include "GSH_VulkanDraw.h"
#include "MemStream.h"
#include "vulkan/StructDefs.h"
#include "nuanceur/Builder.h"
#include "nuanceur/generators/SpirvShaderGenerator.h"

using namespace GSH_Vulkan;

#define DRAW_AREA_SIZE 1024

CDraw::CDraw(const ContextPtr& context)
	: m_context(context)
{
	CreateRenderPass();
	CreateFramebuffer();
	CreateVertexShader();
	CreateFragmentShader();
	CreateDrawPipeline();
}

CDraw::~CDraw()
{
	m_context->device.vkDestroyPipeline(m_context->device, m_drawPipeline, nullptr);
	m_context->device.vkDestroyPipelineLayout(m_context->device, m_drawPipelineLayout, nullptr);
	m_context->device.vkDestroyFramebuffer(m_context->device, m_framebuffer, nullptr);
	m_context->device.vkDestroyRenderPass(m_context->device, m_renderPass, nullptr);
}

void CDraw::AddVertices(const PRIM_VERTEX* vertexBeginPtr, const PRIM_VERTEX* vertexEndPtr)
{
	m_primVertices.insert(m_primVertices.end(), vertexBeginPtr, vertexEndPtr);
}

void CDraw::FlushVertices()
{
	if(m_primVertices.empty()) return;

	auto result = VK_SUCCESS;
	auto commandBuffer = m_context->commandBufferPool.AllocateBuffer();

	auto commandBufferBeginInfo = Framework::Vulkan::CommandBufferBeginInfo();
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	result = m_context->device.vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
	CHECKVULKANERROR(result);

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

	auto renderPassBeginInfo = Framework::Vulkan::RenderPassBeginInfo();
	renderPassBeginInfo.renderPass               = m_renderPass;
	renderPassBeginInfo.renderArea.extent.width  = DRAW_AREA_SIZE;
	renderPassBeginInfo.renderArea.extent.height = DRAW_AREA_SIZE;
	renderPassBeginInfo.framebuffer              = m_framebuffer;
	m_context->device.vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	m_context->device.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_drawPipeline);

	//VkDeviceSize vertexBufferOffset = 0;
	//m_context->device.vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_vertexBuffer, &vertexBufferOffset);

	m_context->device.vkCmdDraw(commandBuffer, 3, 1, 0, 0);

	m_context->device.vkCmdEndRenderPass(commandBuffer);
	m_context->device.vkEndCommandBuffer(commandBuffer);

	m_primVertices.clear();
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
		
	auto result = m_context->device.vkCreateFramebuffer(m_context->device, &frameBufferCreateInfo, nullptr, &m_framebuffer);
	CHECKVULKANERROR(result);
}

void CDraw::CreateRenderPass()
{
	assert(m_renderPass == VK_NULL_HANDLE);
	
	auto result = VK_SUCCESS;
		
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	
	auto renderPassCreateInfo = Framework::Vulkan::RenderPassCreateInfo();
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses   = &subpass;

	result = m_context->device.vkCreateRenderPass(m_context->device, &renderPassCreateInfo, nullptr, &m_renderPass);
	CHECKVULKANERROR(result);
}

void CDraw::CreateDrawPipeline()
{
	assert(!m_vertexShader.IsEmpty());
	assert(!m_fragmentShader.IsEmpty());
	assert(m_drawPipeline == VK_NULL_HANDLE);
	assert(m_drawPipelineLayout == VK_NULL_HANDLE);
//	assert(m_drawDescriptorSetLayout == VK_NULL_HANDLE);

	auto result = VK_SUCCESS;

	{
		auto pipelineLayoutCreateInfo = Framework::Vulkan::PipelineLayoutCreateInfo();
		//pipelineLayoutCreateInfo.setLayoutCount = 1;
		//pipelineLayoutCreateInfo.pSetLayouts    = &m_drawDescriptorSetLayout;

		result = m_context->device.vkCreatePipelineLayout(m_context->device, &pipelineLayoutCreateInfo, nullptr, &m_drawPipelineLayout);
		CHECKVULKANERROR(result);
	}

	auto inputAssemblyInfo = Framework::Vulkan::PipelineInputAssemblyStateCreateInfo();
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	
	std::vector<VkVertexInputAttributeDescription> vertexAttributes;

	{
		VkVertexInputAttributeDescription positionVertexAttributeDesc = {};
		positionVertexAttributeDesc.format = VK_FORMAT_R32G32_SFLOAT;
		positionVertexAttributeDesc.offset = offsetof(PRIM_VERTEX, x);
		positionVertexAttributeDesc.location = 0;
		vertexAttributes.push_back(positionVertexAttributeDesc);
	}

	{
		VkVertexInputAttributeDescription texCoordVertexAttributeDesc = {};
		texCoordVertexAttributeDesc.format = VK_FORMAT_R32_UINT;
		texCoordVertexAttributeDesc.offset = offsetof(PRIM_VERTEX, z);
		texCoordVertexAttributeDesc.location = 1;
		vertexAttributes.push_back(texCoordVertexAttributeDesc);
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
		auto inputPosition = CFloat4Lvalue(b.CreateInput(Nuanceur::SEMANTIC_POSITION));
		auto outputPosition = CFloat4Lvalue(b.CreateOutput(Nuanceur::SEMANTIC_SYSTEM_POSITION));
		outputPosition = NewFloat4(inputPosition->xyz(), NewFloat(b, 1.0f));
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
		auto outputColor = CFloat4Lvalue(b.CreateOutput(Nuanceur::SEMANTIC_SYSTEM_COLOR));
		outputColor = NewFloat4(b, 255.f, 255.f, 255.f, 255.f);
	}
	
	Framework::CMemStream shaderStream;
	//Framework::CStdStream shaderStream("frag.spv", "wb");
	Nuanceur::CSpirvShaderGenerator::Generate(shaderStream, b, Nuanceur::CSpirvShaderGenerator::SHADER_TYPE_FRAGMENT);
	shaderStream.Seek(0, Framework::STREAM_SEEK_SET);
	m_fragmentShader = Framework::Vulkan::CShaderModule(m_context->device, shaderStream);
}
