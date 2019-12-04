#include "GSH_VulkanTransfer.h"
#include "GSH_VulkanMemoryUtils.h"
#include "MemStream.h"
#include "vulkan/StructDefs.h"
#include "vulkan/Utils.h"
#include "nuanceur/generators/SpirvShaderGenerator.h"
#include "../GSHandler.h"

using namespace GSH_Vulkan;

#define XFER_BUFFER_SIZE 0x400000

#define DESCRIPTOR_LOCATION_MEMORY 0
#define DESCRIPTOR_LOCATION_XFERBUFFER 1

#define LOCAL_SIZE_X 128

CTransfer::CTransfer(const ContextPtr& context)
	: m_context(context)
{
	CreateXferBuffer();
	m_pipelineCaps <<= 0;
}

CTransfer::~CTransfer()
{
	for(const auto& xferPipelinePair : m_xferPipelines)
	{
		auto& xferPipeline = xferPipelinePair.second;
		m_context->device.vkDestroyPipeline(m_context->device, xferPipeline.pipeline, nullptr);
		m_context->device.vkDestroyPipelineLayout(m_context->device, xferPipeline.pipelineLayout, nullptr);
		m_context->device.vkDestroyDescriptorSetLayout(m_context->device, xferPipeline.descriptorSetLayout, nullptr);
	}
	m_context->device.vkDestroyBuffer(m_context->device, m_xferBuffer, nullptr);
	m_context->device.vkFreeMemory(m_context->device, m_xferBufferMemory, nullptr);
}

void CTransfer::SetPipelineCaps(const PIPELINE_CAPS& pipelineCaps)
{
	m_pipelineCaps = pipelineCaps;
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

	//Find pipeline and create it if we've never encountered it before
	auto xferPipelinePairIterator = m_xferPipelines.find(m_pipelineCaps);
	if(xferPipelinePairIterator == std::end(m_xferPipelines))
	{
		auto xferPipeline = CreateXferPipeline(m_pipelineCaps);
		m_xferPipelines.insert(std::make_pair(m_pipelineCaps, xferPipeline));
		xferPipelinePairIterator = m_xferPipelines.find(m_pipelineCaps);
	}

	const auto& xferPipeline = xferPipelinePairIterator->second;

	uint32 pixelCount = 0;
	switch(m_pipelineCaps.dstFormat)
	{
	default:
		//assert(false);
	case CGSHandler::PSMCT32:
		pixelCount = inputData.size() / 4;
		break;
	case CGSHandler::PSMT8:
		pixelCount = inputData.size();
		break;
	}

	uint32 workUnits = pixelCount / LOCAL_SIZE_X;

	auto descriptorSet = PrepareDescriptorSet(xferPipeline.descriptorSetLayout);
	auto commandBuffer = m_context->commandBufferPool.AllocateBuffer();

	auto commandBufferBeginInfo = Framework::Vulkan::CommandBufferBeginInfo();
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	result = m_context->device.vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
	CHECKVULKANERROR(result);

	m_context->device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, xferPipeline.pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	m_context->device.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, xferPipeline.pipeline);
	m_context->device.vkCmdPushConstants(commandBuffer, xferPipeline.pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(XFERPARAMS), &Params);
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

	result = m_context->device.vkQueueWaitIdle(m_context->queue);
	CHECKVULKANERROR(result);
}

VkDescriptorSet CTransfer::PrepareDescriptorSet(VkDescriptorSetLayout descriptorSetLayout)
{
	VkResult result = VK_SUCCESS;
	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

	//Allocate descriptor set
	{
		auto setAllocateInfo = Framework::Vulkan::DescriptorSetAllocateInfo();
		setAllocateInfo.descriptorPool     = m_context->descriptorPool;
		setAllocateInfo.descriptorSetCount = 1;
		setAllocateInfo.pSetLayouts        = &descriptorSetLayout;

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

Framework::Vulkan::CShaderModule CTransfer::CreateXferShader(const PIPELINE_CAPS& caps)
{
	using namespace Nuanceur;
	
	auto b = CShaderBuilder();
	
	b.SetMetadata(CShaderBuilder::METADATA_LOCALSIZE_X, LOCAL_SIZE_X);

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

		auto pixelIndex = inputInvocationId->x();

		switch(caps.dstFormat)
		{
		case CGSHandler::PSMCT32:
			{
				auto input = XferStream_Read32(b, xferBuffer, pixelIndex);
				auto address = CMemoryUtils::GetPixelAddress_PSMCT32(b, bufAddress, bufWidth, NewInt2(trxX, trxY));
				CMemoryUtils::Memory_Write32(b, memoryImage, address, input);
			}
			break;
		case CGSHandler::PSMT8:
			{
				auto input = XferStream_Read8(b, xferBuffer, pixelIndex);
				auto address = CMemoryUtils::GetPixelAddress_PSMT8(b, bufAddress, bufWidth, NewInt2(trxX, trxY));
				CMemoryUtils::Memory_Write8(b, memoryImage, address, input);
			}
			break;
		default:
			assert(false);
			break;
		}
	}
	
	Framework::CMemStream shaderStream;
	Nuanceur::CSpirvShaderGenerator::Generate(shaderStream, b, Nuanceur::CSpirvShaderGenerator::SHADER_TYPE_COMPUTE);
	shaderStream.Seek(0, Framework::STREAM_SEEK_SET);
	return Framework::Vulkan::CShaderModule(m_context->device, shaderStream);
}

Nuanceur::CUintRvalue CTransfer::XferStream_Read32(Nuanceur::CShaderBuilder& b, Nuanceur::CArrayUintValue xferBuffer, Nuanceur::CIntValue pixelIndex)
{
	return Load(xferBuffer, pixelIndex);
}

Nuanceur::CUintRvalue CTransfer::XferStream_Read8(Nuanceur::CShaderBuilder& b, Nuanceur::CArrayUintValue xferBuffer, Nuanceur::CIntValue pixelIndex)
{
	auto srcOffset = pixelIndex / NewInt(b, 4);
	auto srcShift = (ToUint(pixelIndex) & NewUint(b, 3)) * NewUint(b, 8);
	return (Load(xferBuffer, srcOffset) >> srcShift) & NewUint(b, 0xFF);
}

CTransfer::XFER_PIPELINE CTransfer::CreateXferPipeline(const PIPELINE_CAPS& caps)
{
	XFER_PIPELINE xferPipeline;

	auto xferShader = CreateXferShader(caps);

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
		result = m_context->device.vkCreateDescriptorSetLayout(m_context->device, &createInfo, nullptr, &xferPipeline.descriptorSetLayout);
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
		pipelineLayoutCreateInfo.pSetLayouts            = &xferPipeline.descriptorSetLayout;

		result = m_context->device.vkCreatePipelineLayout(m_context->device, &pipelineLayoutCreateInfo, nullptr, &xferPipeline.pipelineLayout);
		CHECKVULKANERROR(result);
	}

	{
		auto createInfo = Framework::Vulkan::ComputePipelineCreateInfo();
		createInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		createInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		createInfo.stage.pName = "main";
		createInfo.stage.module = xferShader;
		createInfo.layout = xferPipeline.pipelineLayout;

		result = m_context->device.vkCreateComputePipelines(m_context->device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &xferPipeline.pipeline);
		CHECKVULKANERROR(result);
	}

	return xferPipeline;
}
