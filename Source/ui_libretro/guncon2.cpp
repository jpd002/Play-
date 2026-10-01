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
        guncon2_set_state(vm,instance,buttons,0,0,true);
        return;
    }
    
	int32 screenX = static_cast<int32>(g_input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_SCREEN_X));
	int32 screenY = static_cast<int32>(g_input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_SCREEN_Y));
    
	int32 x = ( (screenX * g_lightgun_info->width) / 0x100 * g_lightgun_info->scale_x + 0x100 * 5000) / (0x100 * 10000)
                    + g_lightgun_info->center_x;
	int32 y = ( (screenY * g_lightgun_info->height) / 0x100 * g_lightgun_info->scale_y + 0x100 * 5000) / (0x100 * 10000)
                    + g_lightgun_info->center_y;
                    
    guncon2_set_state(vm,instance,buttons,x,y,false);
}

void update_guns(CPS2VM* vm) 
{
    for (unsigned port=0; port<MAX_GUNS; port++)
        if (port_is_gun[port])
            update_gun(vm, port);
}

void register_guns(CPS2VM* vm, bool padMode) 
{
    int needed = 0;
    for (int port=0; port<MAX_GUNS; port++)
        if (port_is_gun[port]) 
            needed++;
        
    // TODO: if a gun is unregistered, remove it somehow
    for (int i=0; i<needed; i++) 
        register_guncon2(vm, i, padMode);
    
    if (registered < needed)
        registered = needed;
}
