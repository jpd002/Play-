#include "GSH_VulkanTransferHost.h"
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
#define DESCRIPTOR_LOCATION_MEMORY_8BIT 3
#define DESCRIPTOR_LOCATION_MEMORY_16BIT 4

#define LOCAL_SIZE_X 1024

CTransferHost::CTransferHost(const ContextPtr& context, const FrameCommandBufferPtr& frameCommandBuffer)
    : m_context(context)
    , m_frameCommandBuffer(frameCommandBuffer)
    , m_pipelineCache(context->device)
{
	for(auto& frame : m_frames)
	{
		frame.xferBuffer = Framework::Vulkan::CBuffer(
		    m_context->device, m_context->physicalDeviceMemoryProperties,
		    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		    XFER_BUFFER_SIZE);

		auto result = m_context->device.vkMapMemory(m_context->device, frame.xferBuffer.GetMemory(),
		                                            0, VK_WHOLE_SIZE, 0, reinterpret_cast<void**>(&frame.xferBufferPtr));
		CHECKVULKANERROR(result);
	}

	m_pipelineCaps <<= 0;
}

CTransferHost::~CTransferHost()
{
	for(auto& frame : m_frames)
	{
		m_context->device.vkUnmapMemory(m_context->device, frame.xferBuffer.GetMemory());
	}
}

void CTransferHost::SetPipelineCaps(const PIPELINE_CAPS& pipelineCaps)
{
	m_pipelineCaps = pipelineCaps;
}

void CTransferHost::DoTransfer(const XferBuffer& inputData)
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
	case CGSHandler::PSMZ32:
		pixelCount = inputData.size() / 4;
		break;
	case CGSHandler::PSMCT24:
	case CGSHandler::PSMZ24:
		pixelCount = inputData.size() / 3;
		break;
	case CGSHandler::PSMCT16S:
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

	//Add a barrier to ensure reads are complete before writing to GS memory
	{
		auto memoryBarrier = Framework::Vulkan::MemoryBarrier();
		memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

		m_context->device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		                                       0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
	}

	m_context->device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, xferPipeline->pipelineLayout, 0, 1, &descriptorSet, 1, &m_xferBufferOffset);
	m_context->device.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, xferPipeline->pipeline);
	m_context->device.vkCmdPushConstants(commandBuffer, xferPipeline->pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(XFERPARAMS), &Params);
	m_context->device.vkCmdDispatch(commandBuffer, workUnits, 1, 1);

	m_xferBufferOffset += inputData.size();
}

VkDescriptorSet CTransferHost::PrepareDescriptorSet(VkDescriptorSetLayout descriptorSetLayout, const DESCRIPTORSET_CAPS& caps)
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

		//Memory Image Descriptor 8 bit
		if(false)
		{
			auto writeSet = Framework::Vulkan::WriteDescriptorSet();
			writeSet.dstSet = descriptorSet;
			writeSet.dstBinding = DESCRIPTOR_LOCATION_MEMORY_8BIT;
			writeSet.descriptorCount = 1;
			writeSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			writeSet.pBufferInfo = &descriptorMemoryBufferInfo;
			writes.push_back(writeSet);
		}

		//Memory Image Descriptor 16 bit
		if(false)
		{
			auto writeSet = Framework::Vulkan::WriteDescriptorSet();
			writeSet.dstSet = descriptorSet;
			writeSet.dstBinding = DESCRIPTOR_LOCATION_MEMORY_16BIT;
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

void CTransferHost::PreFlushFrameCommandBuffer()
{
}

void CTransferHost::PostFlushFrameCommandBuffer()
{
	m_xferBufferOffset = 0;
}

Framework::Vulkan::CShaderModule CTransferHost::CreateXferShader(const PIPELINE_CAPS& caps)
{
	using namespace Nuanceur;

	auto b = CShaderBuilder();

	b.SetMetadata(CShaderBuilder::METADATA_LOCALSIZE_X, LOCAL_SIZE_X);

	{
		auto inputInvocationId = CInt4Lvalue(b.CreateInputInt(Nuanceur::SEMANTIC_SYSTEM_GIID));
		auto memoryBuffer = CArrayUintValue(b.CreateUniformArrayUint("memoryBuffer", DESCRIPTOR_LOCATION_MEMORY));
		//auto memoryBuffer8 = CArrayUcharValue(b.CreateUniformArrayUchar("memoryBuffer8", DESCRIPTOR_LOCATION_MEMORY_8BIT));
		//auto memoryBuffer16 = CArrayUshortValue(b.CreateUniformArrayUshort("memoryBuffer16", DESCRIPTOR_LOCATION_MEMORY_16BIT));
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
		case CGSHandler::PSMZ32:
		{
			auto input = XferStream_Read32(b, xferBuffer, pixelIndex);
			auto address = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMCT32>(
			    b, dstSwizzleTable, bufAddress, bufWidth, NewInt2(trxX, trxY));
			CMemoryUtils::Memory_Write32(b, memoryBuffer, address, input);
		}
		break;
		case CGSHandler::PSMCT24:
		case CGSHandler::PSMZ24:
		{
			auto input = XferStream_Read24(b, xferBuffer, pixelIndex);
			auto address = CMemoryUtils::GetPixelAddress<CGsPixelFormats::STORAGEPSMCT32>(
			    b, dstSwizzleTable, bufAddress, bufWidth, NewInt2(trxX, trxY));
			CMemoryUtils::Memory_Write24(b, memoryBuffer, address, input);
		}
		break;
		case CGSHandler::PSMCT16S:
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

Nuanceur::CUintRvalue CTransferHost::XferStream_Read32(Nuanceur::CShaderBuilder& b, Nuanceur::CArrayUintValue xferBuffer, Nuanceur::CIntValue pixelIndex)
{
	return Load(xferBuffer, pixelIndex);
}

Nuanceur::CUintRvalue CTransferHost::XferStream_Read24(Nuanceur::CShaderBuilder& b, Nuanceur::CArrayUintValue xferBuffer, Nuanceur::CIntValue pixelIndex)
{
	auto byteOffset = pixelIndex * NewInt(b, 3);
	auto byte0 = XferStream_Read8(b, xferBuffer, byteOffset + NewInt(b, 0));
	auto byte1 = XferStream_Read8(b, xferBuffer, byteOffset + NewInt(b, 1));
	auto byte2 = XferStream_Read8(b, xferBuffer, byteOffset + NewInt(b, 2));
	return (byte0) | (byte1 << NewUint(b, 8)) | (byte2 << NewUint(b, 16));
}

Nuanceur::CUintRvalue CTransferHost::XferStream_Read16(Nuanceur::CShaderBuilder& b, Nuanceur::CArrayUintValue xferBuffer, Nuanceur::CIntValue pixelIndex)
{
	auto srcOffset = pixelIndex / NewInt(b, 2);
	auto srcShift = (ToUint(pixelIndex) & NewUint(b, 1)) * NewUint(b, 16);
	return (Load(xferBuffer, srcOffset) >> srcShift) & NewUint(b, 0xFFFF);
}

Nuanceur::CUintRvalue CTransferHost::XferStream_Read8(Nuanceur::CShaderBuilder& b, Nuanceur::CArrayUintValue xferBuffer, Nuanceur::CIntValue pixelIndex)
{
	auto srcOffset = pixelIndex / NewInt(b, 4);
	auto srcShift = (ToUint(pixelIndex) & NewUint(b, 3)) * NewUint(b, 8);
	return (Load(xferBuffer, srcOffset) >> srcShift) & NewUint(b, 0xFF);
}

Nuanceur::CUintRvalue CTransferHost::XferStream_Read4(Nuanceur::CShaderBuilder& b, Nuanceur::CArrayUintValue xferBuffer, Nuanceur::CIntValue pixelIndex)
{
	auto srcOffset = pixelIndex / NewInt(b, 8);
	auto srcShift = (ToUint(pixelIndex) & NewUint(b, 7)) * NewUint(b, 4);
	return (Load(xferBuffer, srcOffset) >> srcShift) & NewUint(b, 0xF);
}

PIPELINE CTransferHost::CreateXferPipeline(const PIPELINE_CAPS& caps)
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

		//GS memory - 8bit buffer access
		if(false)
		{
			VkDescriptorSetLayoutBinding binding = {};
			binding.binding = DESCRIPTOR_LOCATION_MEMORY_8BIT;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			binding.descriptorCount = 1;
			binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			bindings.push_back(binding);
		}

		//GS memory - 16bit buffer access
		if(false)
		{
			VkDescriptorSetLayoutBinding binding = {};
			binding.binding = DESCRIPTOR_LOCATION_MEMORY_16BIT;
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
