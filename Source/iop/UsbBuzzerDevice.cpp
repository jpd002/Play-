#include "UsbBuzzerDevice.h"
#include "UsbDefs.h"
#include "IopBios.h"
#include "PadHandler.h"
#include "Ps2Const.h"
#include "states/RegisterState.h"

using namespace Iop;

#define STATE_REG_DESCRIPTORMEMPTR ("descriptorMemPtr")
#define STATE_REG_NEXTTRANSFERTICKS ("nextTransferTicks")
#define STATE_REG_TRANSFERBUFFERPTR ("transferBufferPtr")
#define STATE_REG_TRANSFERSIZE ("transferSize")
#define STATE_REG_TRANSFERCB ("transferCb")
#define STATE_REG_TRANSFERCBARG ("transferCbArg")

CBuzzerUsbDevice::CBuzzerUsbDevice(CIopBios& bios, uint8* ram)
    : m_bios(bios)
    , m_ram(ram)
{
}

void CBuzzerUsbDevice::SaveState(CRegisterState& state) const
{
	state.SetRegister32(STATE_REG_DESCRIPTORMEMPTR, m_descriptorMemPtr);
	state.SetRegister32(STATE_REG_NEXTTRANSFERTICKS, m_nextTransferTicks);
	state.SetRegister32(STATE_REG_TRANSFERBUFFERPTR, m_transferBufferPtr);
	state.SetRegister32(STATE_REG_TRANSFERSIZE, m_transferSize);
	state.SetRegister32(STATE_REG_TRANSFERCB, m_transferCb);
	state.SetRegister32(STATE_REG_TRANSFERCBARG, m_transferCbArg);
}

void CBuzzerUsbDevice::LoadState(const CRegisterState& state)
{
	m_descriptorMemPtr = state.GetRegister32(STATE_REG_DESCRIPTORMEMPTR);
	m_nextTransferTicks = state.GetRegister32(STATE_REG_NEXTTRANSFERTICKS);
	m_transferBufferPtr = state.GetRegister32(STATE_REG_TRANSFERBUFFERPTR);
	m_transferSize = state.GetRegister32(STATE_REG_TRANSFERSIZE);
	m_transferCb = state.GetRegister32(STATE_REG_TRANSFERCB);
	m_transferCbArg = state.GetRegister32(STATE_REG_TRANSFERCBARG);

	if(!m_padHandler->HasListener(this))
	{
		m_padHandler->InsertListener(this);
	}
}

void CBuzzerUsbDevice::SetPadHandler(CPadHandler* padHandler)
{
	m_padHandler = padHandler;
}

uint16 CBuzzerUsbDevice::GetId() const
{
	return DEVICE_ID;
}

const char* CBuzzerUsbDevice::GetLldName() const
{
	return "buzzer";
}

void CBuzzerUsbDevice::CountTicks(uint32 ticks)
{
	if(m_nextTransferTicks != 0)
	{
		m_nextTransferTicks -= ticks;
		if(m_nextTransferTicks <= 0)
		{
			uint8* buffer = m_ram + m_transferBufferPtr;
			buffer[0] = 0x7F;
			buffer[1] = 0x7F;
			buffer[2] = m_buttonState;
			buffer[3] = 0x00;
			buffer[4] = 0xF0;
			m_bios.TriggerCallback(m_transferCb, 0, m_transferSize, m_transferCbArg);
			m_nextTransferTicks = 0;
			m_transferCb = 0;
		}
	}
}

void CBuzzerUsbDevice::SetButtonState(unsigned int padNumber, PS2::CControllerInfo::BUTTON button, bool pressed, uint8* ram)
{
	if(padNumber == 0)
	{
		//Reference for data transfer:
		//https://gist.github.com/Lewiscowles1986/eef220dac6f0549e4702393a7b9351f6
		uint8 mask = 0;
		switch(button)
		{
		case PS2::CControllerInfo::CROSS:
			mask = 1; //Red
			break;
		case PS2::CControllerInfo::CIRCLE:
			mask = 2; //Yellow
			break;
		case PS2::CControllerInfo::SQUARE:
			mask = 4; //Green
			break;
		case PS2::CControllerInfo::TRIANGLE:
			mask = 8; //Orange
			break;
		case PS2::CControllerInfo::DPAD_UP:
			mask = 0x10; //Blue
			break;
		}
		if(mask != 0)
		{
			m_buttonState &= ~mask;
			if(pressed) m_buttonState |= mask;
		}
	}
}

void CBuzzerUsbDevice::OnLldRegistered()
{
	m_descriptorMemPtr = m_bios.GetSysmem()->AllocateMemory(0x80, 0, 0);
	m_padHandler->InsertListener(this);
}

uint32 CBuzzerUsbDevice::ScanStaticDescriptor(uint32 deviceId, uint32 descriptorPtr, uint32 descriptorType)
{
	assert(deviceId == DEVICE_ID);
	uint32 result = 0;
	switch(descriptorType)
	{
	case Usb::DESCRIPTOR_TYPE_DEVICE:
	{
		auto descriptor = reinterpret_cast<Usb::DEVICE_DESCRIPTOR*>(m_ram + m_descriptorMemPtr);
		descriptor->base.descriptorType = Usb::DESCRIPTOR_TYPE_DEVICE;
		result = m_descriptorMemPtr;
	}
	break;
	case Usb::DESCRIPTOR_TYPE_CONFIGURATION:
	{
		auto descriptor = reinterpret_cast<Usb::CONFIGURATION_DESCRIPTOR*>(m_ram + m_descriptorMemPtr);
		descriptor->base.descriptorType = Usb::DESCRIPTOR_TYPE_CONFIGURATION;
		descriptor->numInterfaces = 1;
		result = m_descriptorMemPtr;
	}
	break;
	case Usb::DESCRIPTOR_TYPE_INTERFACE:
	{
		auto descriptor = reinterpret_cast<Usb::INTERFACE_DESCRIPTOR*>(m_ram + m_descriptorMemPtr);
		descriptor->base.descriptorType = Usb::DESCRIPTOR_TYPE_INTERFACE;
		descriptor->numEndpoints = 1;
		result = m_descriptorMemPtr;
	}
	break;
	case Usb::DESCRIPTOR_TYPE_ENDPOINT:
	{
		auto descriptor = reinterpret_cast<Usb::ENDPOINT_DESCRIPTOR*>(m_ram + m_descriptorMemPtr);
		if(descriptor->base.descriptorType != Usb::DESCRIPTOR_TYPE_ENDPOINT)
		{
			descriptor->base.descriptorType = Usb::DESCRIPTOR_TYPE_ENDPOINT;
			descriptor->endpointAddress = 0x80;
			descriptor->attributes = 3; //Interrupt transfer type
			result = m_descriptorMemPtr;
		}
	}
	break;
	}
	return result;
}

int32 CBuzzerUsbDevice::OpenPipe(uint32 deviceId, uint32 descriptorPtr)
{
	assert(deviceId == DEVICE_ID);
	if(descriptorPtr != 0)
	{
		assert(descriptorPtr == m_descriptorMemPtr);
		return PIPE_ID;
	}
	else
	{
		return CONTROL_PIPE_ID;
	}
}

int32 CBuzzerUsbDevice::TransferPipe(uint32 pipeId, uint32 bufferPtr, uint32 size, uint32 optionPtr, uint32 doneCb, uint32 arg)
{
	uint16 deviceId = (pipeId & 0xFFFF);
	uint16 internalPipeId = (pipeId >> 16) & 0xFFF;
	assert(deviceId == DEVICE_ID);
	switch(internalPipeId)
	{
	case CONTROL_PIPE_ID:
		m_bios.TriggerCallback(doneCb, 0, size, arg);
		return 0;
		break;
	case PIPE_ID:
		//Interrupt transfer
		m_transferBufferPtr = bufferPtr;
		m_transferSize = size;
		m_transferCb = doneCb;
		m_transferCbArg = arg;
		m_nextTransferTicks = PS2::IOP_CLOCK_OVER_FREQ / 60;
		return 0;
	default:
		assert(false);
		return -1;
	}
}
