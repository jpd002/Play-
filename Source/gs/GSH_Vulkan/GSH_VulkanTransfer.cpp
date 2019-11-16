#include "GSH_VulkanTransfer.h"
#include "MemStream.h"
#include "vulkan/StructDefs.h"
#include "vulkan/Utils.h"
#include "nuanceur/Builder.h"
#include "nuanceur/generators/SpirvShaderGenerator.h"

using namespace GSH_Vulkan;

#define XFER_BUFFER_SIZE 0x400000

#define DESCRIPTOR_LOCATION_MEMORY 0
#define DESCRIPTOR_LOCATION_XFERBUFFER 1

CTransfer::CTransfer(const ContextPtr& context)
	: m_context(context)
{
	CreateXferBuffer();
	CreateXferShader();
	CreatePipeline();
}

CTransfer::~CTransfer()
{
	m_context->device.vkDestroyPipeline(m_context->device, m_pipeline, nullptr);
	m_context->device.vkDestroyPipelineLayout(m_context->device, m_pipelineLayout, nullptr);
	m_context->device.vkDestroyDescriptorSetLayout(m_context->device, m_descriptorSetLayout, nullptr);
	m_context->device.vkDestroyBuffer(m_context->device, m_xferBuffer, nullptr);
	m_context->device.vkFreeMemory(m_context->device, m_xferBufferMemory, nullptr);
}

void CTransfer::DoHostToLocalTransfer(const XferBuffer& inputData)
{
	auto result = VK_SUCCESS;

	//Update xfer buffer
	{
		assert(inputData.size() <= XFER_BUFFER_SIZE);
		void* bufferMemoryData = nullptr;
		auto result = m_context->device.vkMapMemory(m_context->device, m_xferBufferMemory, 0, VK_WHOLE_SIZE, 0, &bufferMemoryData);
		CHECKVULKANERROR(result);
		memcpy(bufferMemoryData, inputData.data(), inputData.size());
		m_context->device.vkUnmapMemory(m_context->device, m_xferBufferMemory);
	}

	uint32 blockCount = inputData.size() / 4;
	uint32 workUnits = blockCount / 128;

	auto descriptorSet = PrepareDescriptorSet();
	auto commandBuffer = m_context->commandBufferPool.AllocateBuffer();

	auto commandBufferBeginInfo = Framework::Vulkan::CommandBufferBeginInfo();
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	result = m_context->device.vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
	CHECKVULKANERROR(result);

	m_context->device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	m_context->device.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
	m_context->device.vkCmdPushConstants(commandBuffer, m_pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(XFERPARAMS), &Params);
	m_context->device.vkCmdDispatch(commandBuffer, workUnits, 1, 1);

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

		VkDescriptorImageInfo descriptorImageInfo = {};
		descriptorImageInfo.imageView = m_context->memoryImageView;
		descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkDescriptorBufferInfo descriptorBufferInfo = {};
		descriptorBufferInfo.buffer = m_xferBuffer;
		descriptorBufferInfo.range = VK_WHOLE_SIZE;

		//Memory Image Descriptor
		{
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
			auto writeSet = Framework::Vulkan::WriteDescriptorSet();
			writeSet.dstSet          = descriptorSet;
			writeSet.dstBinding      = DESCRIPTOR_LOCATION_XFERBUFFER;
			writeSet.descriptorCount = 1;
			writeSet.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			writeSet.pBufferInfo     = &descriptorBufferInfo;
			writes.push_back(writeSet);
		}

		m_context->device.vkUpdateDescriptorSets(m_context->device, writes.size(), writes.data(), 0, nullptr);
	}

	return descriptorSet;
}

void CTransfer::CreateXferBuffer()
{
	VkResult result = VK_SUCCESS;

	auto bufferCreateInfo = Framework::Vulkan::BufferCreateInfo();
	bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	bufferCreateInfo.size  = XFER_BUFFER_SIZE;
	result = m_context->device.vkCreateBuffer(m_context->device, &bufferCreateInfo, nullptr, &m_xferBuffer);
	CHECKVULKANERROR(result);

	VkMemoryRequirements memoryRequirements = {};
	m_context->device.vkGetBufferMemoryRequirements(m_context->device, m_xferBuffer, &memoryRequirements);

	auto memoryAllocateInfo = Framework::Vulkan::MemoryAllocateInfo();
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = Framework::Vulkan::GetMemoryTypeIndex(m_context->physicalDeviceMemoryProperties, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	assert(memoryAllocateInfo.memoryTypeIndex != Framework::Vulkan::VULKAN_MEMORY_TYPE_INVALID);

	result = m_context->device.vkAllocateMemory(m_context->device, &memoryAllocateInfo, nullptr, &m_xferBufferMemory);
	CHECKVULKANERROR(result);
	
	result = m_context->device.vkBindBufferMemory(m_context->device, m_xferBuffer, m_xferBufferMemory, 0);
	CHECKVULKANERROR(result);
}

void CTransfer::CreateXferShader()
{
	using namespace Nuanceur;
	
	auto b = CShaderBuilder();
	
	const uint32 c_memorySize = 1024;

	{
		auto inputInvocationId = CInt4Lvalue(b.CreateInputInt(Nuanceur::SEMANTIC_SYSTEM_GIID));
		auto memoryImage = CImageUint2DValue(b.CreateImage2DUint(DESCRIPTOR_LOCATION_MEMORY));
		auto xferBuffer = CArrayUintValue(b.CreateUniformArrayUint("xferBuffer", DESCRIPTOR_LOCATION_XFERBUFFER));

		auto xferParams0 = CInt4Lvalue(b.CreateUniformInt4("xferParams0", Nuanceur::UNIFORM_UNIT_PUSHCONSTANT));
		auto xferParams1 = CInt4Lvalue(b.CreateUniformInt4("xferParams1", Nuanceur::UNIFORM_UNIT_PUSHCONSTANT));

		auto bufAddress = xferParams0->x();
		auto bufWidth   = xferParams0->y();
		auto rrw        = xferParams0->z();
		auto dsax       = xferParams0->w();
		auto dsay       = xferParams1->x();

		auto rrx = inputInvocationId->x() % rrw;
		auto rry = inputInvocationId->x() / rrw;

		auto trxX = (rrx + dsax) % NewInt(b, 2048);
		auto trxY = (rry + dsay) % NewInt(b, 2048);

		auto inputColor = Load(xferBuffer, inputInvocationId->x());
		auto address = bufAddress + (trxY * bufWidth * NewInt(b, 4)) + (trxX * NewInt(b, 4));

		auto wordAddress = address / NewInt(b, 4);
		auto writePosition = NewInt2(wordAddress % NewInt(b, c_memorySize), wordAddress / NewInt(b, c_memorySize));
		Store(memoryImage, writePosition, NewUint4(inputColor, NewUint3(b, 0, 0, 0)));
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
		VkPushConstantRange pushConstantInfo = {};
		pushConstantInfo.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		pushConstantInfo.offset     = 0;
		pushConstantInfo.size       = sizeof(XFERPARAMS);

		auto pipelineLayoutCreateInfo = Framework::Vulkan::PipelineLayoutCreateInfo();
		pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pipelineLayoutCreateInfo.pPushConstantRanges    = &pushConstantInfo;
		pipelineLayoutCreateInfo.setLayoutCount         = 1;
		pipelineLayoutCreateInfo.pSetLayouts            = &m_descriptorSetLayout;

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
