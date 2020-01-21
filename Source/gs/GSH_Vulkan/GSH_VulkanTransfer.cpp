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
#define DESCRIPTOR_LOCATION_SWIZZLETABLE_DST 2

#define LOCAL_SIZE_X 1024

CTransfer::CTransfer(const ContextPtr& context, const FrameCommandBufferPtr& frameCommandBuffer)
    : m_context(context)
    , m_frameCommandBuffer(frameCommandBuffer)
    , m_pipelineCache(context->device)
{
	for(auto& frame : m_frames)
	{
		frame.xferBuffer = Framework::Vulkan::CBuffer(
			m_context->device, m_context->physicalDeviceMemoryProperties,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, XFER_BUFFER_SIZE);

		auto result = m_context->device.vkMapMemory(m_context->device, frame.xferBuffer.GetMemory(),
													0, VK_WHOLE_SIZE, 0, reinterpret_cast<void**>(&frame.xferBufferPtr));
		CHECKVULKANERROR(result);
	}

	m_pipelineCaps <<= 0;
}

CTransfer::~CTransfer()
{
	for(auto& frame : m_frames)
	{
		m_context->device.vkUnmapMemory(m_context->device, frame.xferBuffer.GetMemory());
	}
}

void CTransfer::SetPipelineCaps(const PIPELINE_CAPS& pipelineCaps)
{
	m_pipelineCaps = pipelineCaps;
}

void CTransfer::DoHostToLocalTransfer(const XferBuffer& inputData)
{
	auto result = VK_SUCCESS;

	uint32 xferBufferRemainSize = XFER_BUFFER_SIZE - m_xferBufferOffset;
	if(xferBufferRemainSize < inputData.size())
	{
		m_frameCommandBuffer->Flush();
		assert((XFER_BUFFER_SIZE - m_xferBufferOffset) >= inputData.size());
	}

	auto& frame = m_frames[m_frameCommandBuffer->GetCurrentFrame()];

	assert(inputData.size() <= XFER_BUFFER_SIZE);
	memcpy(frame.xferBufferPtr + m_xferBufferOffset, inputData.data(), inputData.size());

	//Find pipeline and create it if we've never encountered it before
	auto xferPipeline = m_pipelineCache.TryGetPipeline(m_pipelineCaps);
	if(!xferPipeline)
	{
		xferPipeline = m_pipelineCache.RegisterPipeline(m_pipelineCaps, CreateXferPipeline(m_pipelineCaps));
	}

	uint32 pixelCount = 0;
	switch(m_pipelineCaps.dstFormat)
	{
	default:
		assert(false);
	case CGSHandler::PSMCT32:
		pixelCount = inputData.size() / 4;
		break;
	case CGSHandler::PSMCT16:
		pixelCount = inputData.size() / 2;
		break;
	case CGSHandler::PSMT8:
	case CGSHandler::PSMT8H:
		pixelCount = inputData.size();
		break;
	case CGSHandler::PSMT4:
	case CGSHandler::PSMT4HL:
	case CGSHandler::PSMT4HH:
		pixelCount = inputData.size() * 2;
		break;
	}

	Params.pixelCount = pixelCount;
	uint32 workUnits = (pixelCount + LOCAL_SIZE_X - 1) / LOCAL_SIZE_X;

	auto descriptorSetCaps = make_convertible<DESCRIPTORSET_CAPS>(0);
	descriptorSetCaps.dstPsm = m_pipelineCaps.dstFormat;
	descriptorSetCaps.frameIdx = m_frameCommandBuffer->GetCurrentFrame();

	auto descriptorSet = PrepareDescriptorSet(xferPipeline->descriptorSetLayout, descriptorSetCaps);
	auto commandBuffer = m_frameCommandBuffer->GetCommandBuffer();

	m_context->device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, xferPipeline->pipelineLayout, 0, 1, &descriptorSet, 1, &m_xferBufferOffset);
	m_context->device.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, xferPipeline->pipeline);
	m_context->device.vkCmdPushConstants(commandBuffer, xferPipeline->pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(XFERPARAMS), &Params);
	m_context->device.vkCmdDispatch(commandBuffer, workUnits, 1, 1);

	m_xferBufferOffset += inputData.size();
}

VkDescriptorSet CTransfer::PrepareDescriptorSet(VkDescriptorSetLayout descriptorSetLayout, const DESCRIPTORSET_CAPS& caps)
{
	auto descriptorSetIterator = m_descriptorSetCache.find(caps);
	if(descriptorSetIterator != std::end(m_descriptorSetCache))
	{
		return descriptorSetIterator->second;
	}

	VkResult result = VK_SUCCESS;
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
		std::vector<VkWriteDescriptorSet> writes;

		VkDescriptorBufferInfo descriptorMemoryBufferInfo = {};
		descriptorMemoryBufferInfo.buffer = m_context->memoryBuffer;
		descriptorMemoryBufferInfo.range = VK_WHOLE_SIZE;

		VkDescriptorBufferInfo descriptorBufferInfo = {};
		descriptorBufferInfo.buffer = m_frames[caps.frameIdx].xferBuffer;
		descriptorBufferInfo.range = VK_WHOLE_SIZE;

		VkDescriptorImageInfo descriptorDstSwizzleTableInfo = {};
		descriptorDstSwizzleTableInfo.imageView = m_context->GetSwizzleTable(caps.dstPsm);
		descriptorDstSwizzleTableInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		//Memory Image Descriptor
		{
			auto writeSet = Framework::Vulkan::WriteDescriptorSet();
			writeSet.dstSet = descriptorSet;
			writeSet.dstBinding = DESCRIPTOR_LOCATION_MEMORY;
			writeSet.descriptorCount = 1;
			writeSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			writeSet.pBufferInfo = &descriptorMemoryBufferInfo;
			writes.push_back(writeSet);
		}

		//Xfer Buffer Descriptor
		{
			auto writeSet = Framework::Vulkan::WriteDescriptorSet();
			writeSet.dstSet = descriptorSet;
			writeSet.dstBinding = DESCRIPTOR_LOCATION_XFERBUFFER;
			writeSet.descriptorCount = 1;
			writeSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
			writeSet.pBufferInfo = &descriptorBufferInfo;
			writes.push_back(writeSet);
		}

		//Dst Swizzle Table
		{
			auto writeSet = Framework::Vulkan::WriteDescriptorSet();
			writeSet.dstSet = descriptorSet;
			writeSet.dstBinding = DESCRIPTOR_LOCATION_SWIZZLETABLE_DST;
			writeSet.descriptorCount = 1;
			writeSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			writeSet.pImageInfo = &descriptorDstSwizzleTableInfo;
			writes.push_back(writeSet);
		}

		m_context->device.vkUpdateDescriptorSets(m_context->device, writes.size(), writes.data(), 0, nullptr);
	}

	m_descriptorSetCache.insert(std::make_pair(caps, descriptorSet));

	return descriptorSet;
}

void CTransfer::PreFlushFrameCommandBuffer()
{

}

void CTransfer::PostFlushFrameCommandBuffer()
{
	m_xferBufferOffset = 0;
}

Framework::Vulkan::CShaderModule CTransfer::CreateXferShader(const PIPELINE_CAPS& caps)
{
	using namespace Nuanceur;

	auto b = CShaderBuilder();

	b.SetMetadata(CShaderBuilder::METADATA_LOCALSIZE_X, LOCAL_SIZE_X);

	{
		auto inputInvocationId = CInt4Lvalue(b.CreateInputInt(Nuanceur::SEMANTIC_SYSTEM_GIID));
		auto memoryBuffer = CArrayUintValue(b.CreateUniformArrayUint("memoryBuffer", DESCRIPTOR_LOCATION_MEMORY));
		auto xferBuffer = CArrayUintValue(b.CreateUniformArrayUint("xferBuffer", DESCRIPTOR_LOCATION_XFERBUFFER));
		auto dstSwizzleTable = CImageUint2DValue(b.CreateImage2DUint(DESCRIPTOR_LOCATION_SWIZZLETABLE_DST));

		auto xferParams0 = CInt4Lvalue(b.CreateUniformInt4("xferParams0", Nuanceur::UNIFORM_UNIT_PUSHCONSTANT));
		auto xferParams1 = CInt4Lvalue(b.CreateUniformInt4("xferParams1", Nuanceur::UNIFORM_UNIT_PUSHCONSTANT));

		auto bufAddress = xferParams0->x();
		auto bufWidth = xferParams0->y();
		auto rrw = xferParams0->z();
		auto dsax = xferParams0->w();
		auto dsay = xferParams1->x();
		auto pixelCount = xferParams1->y();

		auto rrx = inputInvocationId->x() % rrw;
		auto rry = inputInvocationId->x() / rrw;

		auto trxX = (rrx + dsax) % NewInt(b, 2048);
		auto trxY = (rry + dsay) % NewInt(b, 2048);

		auto pixelIndex = inputInvocationId->x();

		BeginIf(b, pixelIndex >= pixelCount);
		{
			Return(b);
		}
		EndIf(b);

		switch(caps.dstFormat)
		{
		case CGSHandler::PSMCT32:
		{
			auto input = XferStream_Read32(b, xferBuffer, pixelIndex);
			auto address = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMCT32>(
			    b, dstSwizzleTable, bufAddress, bufWidth, NewInt2(trxX, trxY));
			CMemoryUtils::Memory_Write32(b, memoryBuffer, address, input);
		}
		break;
		case CGSHandler::PSMCT16:
		{
			auto input = XferStream_Read16(b, xferBuffer, pixelIndex);
			auto address = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMCT16>(
			    b, dstSwizzleTable, bufAddress, bufWidth, NewInt2(trxX, trxY));
			CMemoryUtils::Memory_Write16(b, memoryBuffer, address, input);
		}
		break;
		case CGSHandler::PSMT8:
		{
			auto input = XferStream_Read8(b, xferBuffer, pixelIndex);
			auto address = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMT8>(
			    b, dstSwizzleTable, bufAddress, bufWidth, NewInt2(trxX, trxY));
			CMemoryUtils::Memory_Write8(b, memoryBuffer, address, input);
		}
		break;
		case CGSHandler::PSMT4:
		{
			auto input = XferStream_Read4(b, xferBuffer, pixelIndex);
			auto address = CMemoryUtils::GetPixelAddress_PSMT4(
			    b, dstSwizzleTable, bufAddress, bufWidth, NewInt2(trxX, trxY));
			CMemoryUtils::Memory_Write4(b, memoryBuffer, address, input);
		}
		break;
		case CGSHandler::PSMT8H:
		{
			auto input = XferStream_Read8(b, xferBuffer, pixelIndex);
			auto address = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMCT32>(
			    b, dstSwizzleTable, bufAddress, bufWidth, NewInt2(trxX, trxY));
			CMemoryUtils::Memory_Write8(b, memoryBuffer, address + NewInt(b, 3), input);
		}
		break;
		case CGSHandler::PSMT4HL:
		{
			auto input = XferStream_Read4(b, xferBuffer, pixelIndex);
			auto address = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMCT32>(
			    b, dstSwizzleTable, bufAddress, bufWidth, NewInt2(trxX, trxY));
			auto nibAddress = (address + NewInt(b, 3)) * NewInt(b, 2);
			CMemoryUtils::Memory_Write4(b, memoryBuffer, nibAddress, input);
		}
		break;
		case CGSHandler::PSMT4HH:
		{
			auto input = XferStream_Read4(b, xferBuffer, pixelIndex);
			auto address = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMCT32>(
			    b, dstSwizzleTable, bufAddress, bufWidth, NewInt2(trxX, trxY));
			auto nibAddress = ((address + NewInt(b, 3)) * NewInt(b, 2)) | NewInt(b, 1);
			CMemoryUtils::Memory_Write4(b, memoryBuffer, nibAddress, input);
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

Nuanceur::CUintRvalue CTransfer::XferStream_Read16(Nuanceur::CShaderBuilder& b, Nuanceur::CArrayUintValue xferBuffer, Nuanceur::CIntValue pixelIndex)
{
	auto srcOffset = pixelIndex / NewInt(b, 2);
	auto srcShift = (ToUint(pixelIndex) & NewUint(b, 1)) * NewUint(b, 16);
	return (Load(xferBuffer, srcOffset) >> srcShift) & NewUint(b, 0xFFFF);
}

Nuanceur::CUintRvalue CTransfer::XferStream_Read8(Nuanceur::CShaderBuilder& b, Nuanceur::CArrayUintValue xferBuffer, Nuanceur::CIntValue pixelIndex)
{
	auto srcOffset = pixelIndex / NewInt(b, 4);
	auto srcShift = (ToUint(pixelIndex) & NewUint(b, 3)) * NewUint(b, 8);
	return (Load(xferBuffer, srcOffset) >> srcShift) & NewUint(b, 0xFF);
}

Nuanceur::CUintRvalue CTransfer::XferStream_Read4(Nuanceur::CShaderBuilder& b, Nuanceur::CArrayUintValue xferBuffer, Nuanceur::CIntValue pixelIndex)
{
	auto srcOffset = pixelIndex / NewInt(b, 8);
	auto srcShift = (ToUint(pixelIndex) & NewUint(b, 7)) * NewUint(b, 4);
	return (Load(xferBuffer, srcOffset) >> srcShift) & NewUint(b, 0xF);
}

PIPELINE CTransfer::CreateXferPipeline(const PIPELINE_CAPS& caps)
{
	PIPELINE xferPipeline;

	auto xferShader = CreateXferShader(caps);

	VkResult result = VK_SUCCESS;

	{
		std::vector<VkDescriptorSetLayoutBinding> bindings;

		//GS memory
		{
			VkDescriptorSetLayoutBinding binding = {};
			binding.binding = DESCRIPTOR_LOCATION_MEMORY;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			binding.descriptorCount = 1;
			binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			bindings.push_back(binding);
		}

		//Xfer buffer
		{
			VkDescriptorSetLayoutBinding binding = {};
			binding.binding = DESCRIPTOR_LOCATION_XFERBUFFER;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
			binding.descriptorCount = 1;
			binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			bindings.push_back(binding);
		}

		//Dst Swizzle Table
		{
			VkDescriptorSetLayoutBinding binding = {};
			binding.binding = DESCRIPTOR_LOCATION_SWIZZLETABLE_DST;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			binding.descriptorCount = 1;
			binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			bindings.push_back(binding);
		}

		auto createInfo = Framework::Vulkan::DescriptorSetLayoutCreateInfo();
		createInfo.bindingCount = bindings.size();
		createInfo.pBindings = bindings.data();
		result = m_context->device.vkCreateDescriptorSetLayout(m_context->device, &createInfo, nullptr, &xferPipeline.descriptorSetLayout);
		CHECKVULKANERROR(result);
	}

	{
		VkPushConstantRange pushConstantInfo = {};
		pushConstantInfo.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		pushConstantInfo.offset = 0;
		pushConstantInfo.size = sizeof(XFERPARAMS);

		auto pipelineLayoutCreateInfo = Framework::Vulkan::PipelineLayoutCreateInfo();
		pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pSetLayouts = &xferPipeline.descriptorSetLayout;

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
