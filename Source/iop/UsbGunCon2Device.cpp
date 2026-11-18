#include "UsbGunCon2Device.h"
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

CGunCon2UsbDevice::CGunCon2UsbDevice(CIopBios& bios, uint8* ram, int instance)
    : m_bios(bios)
    , m_ram(ram)
{
    m_instance = instance;
    SetGunInfo(nullptr);
}

void CGunCon2UsbDevice::SaveState(CRegisterState& state) const
{
	state.SetRegister32(STATE_REG_DESCRIPTORMEMPTR, m_descriptorMemPtr);
	state.SetRegister32(STATE_REG_NEXTTRANSFERTICKS, m_nextTransferTicks);
	state.SetRegister32(STATE_REG_TRANSFERBUFFERPTR, m_transferBufferPtr);
	state.SetRegister32(STATE_REG_TRANSFERSIZE, m_transferSize);
	state.SetRegister32(STATE_REG_TRANSFERCB, m_transferCb);
	state.SetRegister32(STATE_REG_TRANSFERCBARG, m_transferCbArg);
}

void CGunCon2UsbDevice::LoadState(const CRegisterState& state)
{
	m_descriptorMemPtr = state.GetRegister32(STATE_REG_DESCRIPTORMEMPTR);
	m_nextTransferTicks = state.GetRegister32(STATE_REG_NEXTTRANSFERTICKS);
	m_transferBufferPtr = state.GetRegister32(STATE_REG_TRANSFERBUFFERPTR);
	m_transferSize = state.GetRegister32(STATE_REG_TRANSFERSIZE);
	m_transferCb = state.GetRegister32(STATE_REG_TRANSFERCB);
	m_transferCbArg = state.GetRegister32(STATE_REG_TRANSFERCBARG);

	if(m_padHandler && !m_padHandler->HasListener(this))
	{
		m_padHandler->InsertListener(this);
	}
}

void CGunCon2UsbDevice::SetPadHandler(CPadHandler* padHandler)
{
	m_padHandler = padHandler;

	if(m_padHandler && !m_padHandler->HasListener(this))
	{
		m_padHandler->InsertListener(this);
	}
}

uint16 CGunCon2UsbDevice::GetId() const
{
	return DEVICE_ID + m_instance;
}

const char* CGunCon2UsbDevice::GetLldName() const
{
	return "usbgun";
}

void CGunCon2UsbDevice::CountTicks(uint32 ticks)
{
	if(m_nextTransferTicks != 0)
	{
		m_nextTransferTicks -= ticks;
		if(m_nextTransferTicks <= 0)
		{
			uint8* buffer = m_ram + m_transferBufferPtr;
            uint16 buttons = ~m_buttonState;
            if (m_progressive)
                buttons &= ~GUN_PROGRESSIVE;
            uint16 x = m_x;
            uint16 y = m_y;
            if ((m_buttonState & GUN_CALIBRATE) && m_calibration_timer == 0) {
                buttons &= ~GUN_TRIGGER;        
                m_calibration_timer = 12;
            }
            else if (m_calibration_timer > 0) {
                buttons &= ~GUN_TRIGGER;
                if (m_calibration_timer < 5) {
                    x = 0;
                    y = 0;
                }
                m_calibration_timer--;
            }
			buffer[0] = buttons & 0xFF;
			buffer[1] = buttons >> 8;
			buffer[2] = x & 0xFF;
			buffer[3] = (x >> 8) & 0xFF;
			buffer[4] = y & 0xFF;
			buffer[5] = (y >> 8) & 0xFF;
			m_bios.TriggerCallback(m_transferCb, 0, m_transferSize, m_transferCbArg);
			m_nextTransferTicks = 0;
			m_transferCb = 0;
		}
	}
}

void CGunCon2UsbDevice::SetScreenPosition(float x, float y)
{
    if (x < -.001f || x >= 1.001f || y < -.001f || y > 1.001f)
    {
        m_x = 0;
        m_y = 0;
        return;
    }
    x = std::clamp(x,0.0f,1.0f);
    y = std::clamp(y,0.0f,1.0f);
    m_x = static_cast<int32>( ( x * m_info.width * m_info.scaleX) + m_info.centerX + 0.5f ) - m_dx;
    m_y = static_cast<int32>( ( y * m_info.height * m_info.scaleY) + m_info.centerY + 0.5f ) - m_dy;
    if (m_x == 0 && m_y == 0)
        m_x = 1; // 0,0 means offscreen
}

void CGunCon2UsbDevice::SetButtonState(unsigned int padNumber, PS2::CControllerInfo::BUTTON button, bool pressed, uint8* ram)
{
	if(padNumber != m_instance)
        return;

    uint32_t mask = 0;
    
    switch(button)
    {
		case PS2::CControllerInfo::CIRCLE:
			mask = GUN_TRIGGER; 
			break;
		case PS2::CControllerInfo::TRIANGLE:
			mask = GUN_A; 
			break;
		case PS2::CControllerInfo::SQUARE:
			mask = GUN_B;
			break;
		case PS2::CControllerInfo::CROSS:
			mask = GUN_C; 
			break;
        case PS2::CControllerInfo::DPAD_UP:
            mask = GUN_UP;
            break;
        case PS2::CControllerInfo::DPAD_RIGHT:
            mask = GUN_RIGHT;
            break;
        case PS2::CControllerInfo::DPAD_LEFT:
            mask = GUN_LEFT;
            break;
        case PS2::CControllerInfo::DPAD_DOWN:
            mask = GUN_DOWN;
            break;
        case PS2::CControllerInfo::SELECT:
            mask = GUN_SELECT;
            break;
        case PS2::CControllerInfo::START:
            mask = GUN_START;
            break;
        case PS2::CControllerInfo::R3:
            mask = GUN_CALIBRATE;
            break;
    }
    
    if (pressed)
        m_buttonState |= mask;
    else
        m_buttonState &= ~mask;
}


void CGunCon2UsbDevice::SetGunButtons(uint32 buttons)
{
    m_buttonState = buttons;
}

void CGunCon2UsbDevice::SetGunInfo(const struct LightgunInfo* infoP)
{
    if (infoP)
    {
        m_info = *infoP;
    }
    else
    {
        m_info = {640,240,1.0f,1.0f,320,120};
    }
}

void CGunCon2UsbDevice::OnLldRegistered()
{
	m_descriptorMemPtr = m_bios.GetSysmem()->AllocateMemory(0x80, 0, 0);

	if(m_padHandler && !m_padHandler->HasListener(this))
	{
		m_padHandler->InsertListener(this);
	}
}

uint32 CGunCon2UsbDevice::ScanStaticDescriptor(uint32 deviceId, uint32 descriptorPtr, uint32 descriptorType)
{
	assert(deviceId == DEVICE_ID + m_instance);
	uint32 result = 0;
	switch(descriptorType)
	{
	case Usb::DESCRIPTOR_TYPE_DEVICE:
	{
		auto descriptor = reinterpret_cast<Usb::DEVICE_DESCRIPTOR*>(m_ram + m_descriptorMemPtr);
		descriptor->base.descriptorType = Usb::DESCRIPTOR_TYPE_DEVICE;
		descriptor->vendorId = 0x0b9a;
		descriptor->productId = 0x016a;
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
			descriptor->maxPacketSize = 8;
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

int32 CGunCon2UsbDevice::OpenPipe(uint32 deviceId, uint32 descriptorPtr)
{
	assert(deviceId == DEVICE_ID + m_instance);
	if(descriptorPtr != 0)
	{
		assert(descriptorPtr == m_descriptorMemPtr);
		return PIPE_ID + m_instance;
	}
	else
	{
		return CONTROL_PIPE_ID + m_instance;
	}
}

void CGunCon2UsbDevice::SetParameters(unsigned char* data) 
{
    m_dx = static_cast<int32>(static_cast<int16>(static_cast<uint16>(data[0]) | (static_cast<uint16>(data[1]) << 8)));
    m_dy = static_cast<int32>(static_cast<int16>(static_cast<uint16>(data[2]) | (static_cast<uint16>(data[3]) << 8)));
    
    m_progressive = ((GUN_PROGRESSIVE>>8) & data[5]) != 0;
    if (m_progressive)
    {
        m_dx /= 2;
        m_dy /= 2;
    }
}

int32 CGunCon2UsbDevice::TransferPipe(uint32 pipeId, uint32 bufferPtr, uint32 size, uint32 optionPtr, uint32 doneCb, uint32 arg)
{
	uint16 deviceId = (pipeId & 0xFFFF);
	uint16 internalPipeId = (pipeId >> 16) & 0xFFF;
	assert(deviceId == DEVICE_ID + m_instance);

	switch(internalPipeId-m_instance)
	{
	case CONTROL_PIPE_ID:
        if (size == 6 && *(unsigned char*)(m_ram+optionPtr) == 0x21 && *(unsigned char*)(m_ram+optionPtr+1) == 0x09) {
            SetParameters((unsigned char*)(m_ram+bufferPtr));
        }
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
