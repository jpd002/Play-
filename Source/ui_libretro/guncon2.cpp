#include "ext/libretro.h"
#include "input/GunCon2Utils.h"
#include "guncon2.h"
#include "PS2VM.h"
#include "iop/Iop_Usbd.h"
#include "iop/UsbGunCon2Device.h"

extern retro_input_state_t g_input_state_cb;
static bool port_is_gun[MAX_GUNS];
static int registered = 0;

void set_gun(unsigned port, bool isGun) 
{
    if (port >= MAX_GUNS)
        isGun = false;
    
    port_is_gun[port] = isGun;
}

static int get_gun_instance(unsigned port)
{
    if (port >= MAX_GUNS)
        return -1;
    
#if MAX_GUNS == 1
    return 0;
#else
    // up two guns are possible; they should be accessed in reverse order
    // dynamic gun insertion is likely to be problematic
    if (port_is_gun[0] && port_is_gun[1]) 
        return 1-port;
    else if (port_is_gun[0])
        return registered-1;
    else 
        return 0;
#endif
    }

static void update_gun(CPS2VM* vm, unsigned port) 
{
    int instance = get_gun_instance(port);
    if (instance < 0)
        return;
    
    uint32_t buttons = 0;
    if (g_input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_AUX_A)) 
        buttons |= Iop::CGunCon2UsbDevice::GUN_A;
    if (g_input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_AUX_B)) 
        buttons |= Iop::CGunCon2UsbDevice::GUN_B;
    if (g_input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_AUX_C)) 
        buttons |= Iop::CGunCon2UsbDevice::GUN_C;
    if (g_input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_DPAD_UP)) 
        buttons |= Iop::CGunCon2UsbDevice::GUN_UP;
    if (g_input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_DPAD_RIGHT)) 
        buttons |= Iop::CGunCon2UsbDevice::GUN_RIGHT;
    if (g_input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_DPAD_DOWN)) 
        buttons |= Iop::CGunCon2UsbDevice::GUN_DOWN;
    if (g_input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_DPAD_LEFT)) 
        buttons |= Iop::CGunCon2UsbDevice::GUN_LEFT;
    if (g_input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_SELECT)) 
        buttons |= Iop::CGunCon2UsbDevice::GUN_SELECT;
    if (g_input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_START)) 
        buttons |= Iop::CGunCon2UsbDevice::GUN_START;
    if (g_input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_START)) 
        buttons |= Iop::CGunCon2UsbDevice::GUN_START;
    if (g_input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_TRIGGER)) 
        buttons |= Iop::CGunCon2UsbDevice::GUN_TRIGGER;
    if (g_input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3)) 
        buttons |= Iop::CGunCon2UsbDevice::GUN_CALIBRATE;

    int offscreen = g_input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_IS_OFFSCREEN);
    int offscreen_shot = g_input_state_cb( port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_RELOAD );
    
    if (offscreen_shot)
        buttons |= Iop::CGunCon2UsbDevice::GUN_TRIGGER;
    
    if (offscreen || offscreen_shot) 
    {
        GunCon2SetState(vm,instance,buttons,-1.0f,-1.0f);
        return;
    }
    
	int32 screenX = static_cast<int32>(g_input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_SCREEN_X));
	int32 screenY = static_cast<int32>(g_input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_SCREEN_Y));
    
	float x = ( screenX + 0x7FFF ) / static_cast<float>(0x10000);
	float y = ( screenY + 0x7FFF ) / static_cast<float>(0x10000);
    
    GunCon2SetState(vm,instance,buttons,x,y);
}

void update_guns(CPS2VM* vm) 
{
    for (unsigned port=0; port<MAX_GUNS; port++)
        if (port_is_gun[port])
            update_gun(vm, port);
}

void register_guns(CPS2VM* vm) 
{
    int needed = 0;
    for (int port=0; port<MAX_GUNS; port++)
        if (port_is_gun[port]) 
            needed++;
        
    const struct Iop::LightgunInfo* infoP = GetLightgunInfo(vm->m_ee->m_os->GetExecutableName());
    // TODO: if a gun is unregistered, remove it somehow

    for (int i=0; i<needed; i++) 
        vm->RegisterGunCon2(i, infoP, false);
    
    if (registered < needed)
        registered = needed;
}
