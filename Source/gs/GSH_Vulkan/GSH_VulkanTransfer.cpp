#include "GSH_VulkanTransfer.h"
#include "MemStream.h"
#include "vulkan/StructDefs.h"
#include "vulkan/Utils.h"
#include "nuanceur/Builder.h"
#include "nuanceur/generators/SpirvShaderGenerator.h"

using namespace GSH_Vulkan;

#define DESCRIPTOR_LOCATION_MEMORY 0
#define DESCRIPTOR_LOCATION_XFERBUFFER 1

CTransfer::CTransfer(const ContextPtr& context)
	: m_context(context)
{
	CreateXferShader();
	CreatePipeline();
}

CTransfer::~CTransfer()
{
	m_xferShader.Reset();
	m_context->device.vkDestroyPipeline(m_context->device, m_pipeline, nullptr);
	m_context->device.vkDestroyPipelineLayout(m_context->device, m_pipelineLayout, nullptr);
	m_context->device.vkDestroyDescriptorSetLayout(m_context->device, m_descriptorSetLayout, nullptr);
}

void CTransfer::DoHostToLocalTransfer()
{
	auto result = VK_SUCCESS;

	auto descriptorSet = PrepareDescriptorSet();
	auto commandBuffer = m_context->commandBufferPool.AllocateBuffer();

	auto commandBufferBeginInfo = Framework::Vulkan::CommandBufferBeginInfo();
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	result = m_context->device.vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
	CHECKVULKANERROR(result);

	m_context->device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	m_context->device.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
	m_context->device.vkCmdDispatch(commandBuffer, 1, 1, 1);

	m_context->device.vkEndCommandBuffer(commandBuffer);

	//Submit command buffer
	{
		auto submitInfo = Framework::Vulkan::SubmitInfo();
		submitInfo.commandBufferCount   = 1;
		submitInfo.pCommandBuffers      = &commandBuffer;
		result = m_context->device.vkQueueSubmit(m_context->queue, 1, &submitInfo, VK_NULL_HANDLE);
		CHECKVULKANERROR(result);
	}
}

VkDescriptorSet CTransfer::PrepareDescriptorSet()
{
	VkResult result = VK_SUCCESS;
	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

	//Allocate descriptor set
	{
		auto setAllocateInfo = Framework::Vulkan::DescriptorSetAllocateInfo();
		setAllocateInfo.descriptorPool     = m_context->descriptorPool;
		setAllocateInfo.descriptorSetCount = 1;
		setAllocateInfo.pSetLayouts        = &m_descriptorSetLayout;

		result = m_context->device.vkAllocateDescriptorSets(m_context->device, &setAllocateInfo, &descriptorSet);
		CHECKVULKANERROR(result);
	}

	//Update descriptor set
	{
		std::vector<VkWriteDescriptorSet> writes;

		//Memory Image Descriptor
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
			writes.push_back(writeSet);
		}

		//Xfer Buffer Descriptor
		{
			VkDescriptorBufferInfo descriptorBufferInfo = {};
			descriptorBufferInfo.buffer = VK_NULL_HANDLE;

			auto writeSet = Framework::Vulkan::WriteDescriptorSet();
			writeSet.dstSet          = descriptorSet;
			writeSet.dstBinding      = DESCRIPTOR_LOCATION_XFERBUFFER;
			writeSet.descriptorCount = 1;
			writeSet.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			writeSet.pBufferInfo     = &descriptorBufferInfo;
			//writes.push_back(writeSet);
		}

		m_context->device.vkUpdateDescriptorSets(m_context->device, writes.size(), writes.data(), 0, nullptr);
	}

	return descriptorSet;
}

void CTransfer::CreateXferShader()
{
	using namespace Nuanceur;
	
	auto b = CShaderBuilder();
	
	{

	}
	
	Framework::CMemStream shaderStream;
	Nuanceur::CSpirvShaderGenerator::Generate(shaderStream, b, Nuanceur::CSpirvShaderGenerator::SHADER_TYPE_COMPUTE);
	shaderStream.Seek(0, Framework::STREAM_SEEK_SET);
	m_xferShader = Framework::Vulkan::CShaderModule(m_context->device, shaderStream);
}

void CTransfer::CreatePipeline()
{
	VkResult result = VK_SUCCESS;

	{
		std::vector<VkDescriptorSetLayoutBinding> bindings;

		//GS memory
		{
			VkDescriptorSetLayoutBinding binding = {};
			binding.binding         = DESCRIPTOR_LOCATION_MEMORY;
			binding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			binding.descriptorCount = 1;
			binding.stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
			bindings.push_back(binding);
		}

		//Xfer buffer
		{
			VkDescriptorSetLayoutBinding binding = {};
			binding.binding         = DESCRIPTOR_LOCATION_XFERBUFFER;
			binding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			binding.descriptorCount = 1;
			binding.stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
			bindings.push_back(binding);
		}

		auto createInfo = Framework::Vulkan::DescriptorSetLayoutCreateInfo();
		createInfo.bindingCount = bindings.size();
		createInfo.pBindings    = bindings.data();
		result = m_context->device.vkCreateDescriptorSetLayout(m_context->device, &createInfo, nullptr, &m_descriptorSetLayout);
		CHECKVULKANERROR(result);
	}

	{
		//VkPushConstantRange pushConstantInfo = {};
		//pushConstantInfo.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		//pushConstantInfo.offset     = 0;
		//pushConstantInfo.size       = sizeof(VERTEX_SHADER_CONSTANTS);

		auto pipelineLayoutCreateInfo = Framework::Vulkan::PipelineLayoutCreateInfo();
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pSetLayouts    = &m_descriptorSetLayout;

		result = m_context->device.vkCreatePipelineLayout(m_context->device, &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout);
		CHECKVULKANERROR(result);
	}

	{
		auto createInfo = Framework::Vulkan::ComputePipelineCreateInfo();
		createInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		createInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		createInfo.stage.pName = "main";
		createInfo.stage.module = m_xferShader;
		createInfo.layout = m_pipelineLayout;

		result = m_context->device.vkCreateComputePipelines(m_context->device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_pipeline);
		CHECKVULKANERROR(result);
	}
}
