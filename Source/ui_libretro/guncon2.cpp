#include "ext/libretro.h"
#include "guncon2.h"
#include "PS2VM.h"
#include "iop/Iop_Usbd.h"
#include "iop/UsbGunCon2Device.h"

extern retro_input_state_t g_input_state_cb;
static bool port_is_gun[MAX_GUNS];
struct lightgun_info_s {
    char serial[10];
    int width;
    int height;    
    int scale_x;
    int scale_y;
    int center_x;
    int center_y;
};

static const struct lightgun_info_s lightgun_defaults = { "default__", 640,240,10000,10000,320,120 };

const struct lightgun_info_s* lightgun_info = nullptr;

static const struct lightgun_info_s lightgun_games[] = {
    {"SLES50930", 640, 256, 9025, 9450, 390, 132}, // Dino Stalker (E, English)
    {"SLES51095", 640, 256, 9025, 9450, 390, 132}, // Dino Stalker (E, French)
    {"SLES51096", 640, 256, 9025, 9450, 390, 132}, // Dino Stalker (E, German)
    {"SLUS20485", 640, 240, 9025, 9250, 390, 132}, // Dino Stalker (U)
    {"SLUS20389", 640, 240, 8925, 9350, 422, 141}, // Endgame (U)
    {"SLES50936", 512, 256, 11200, 10000, 320, 120}, // Endgame (E) (Guncon2 needs to be connected to USB port 2)
    {"SLPM65139", 640, 240, 9000, 9150, 320, 120}, // Gun Survivor 3: Dino Crisis (J)
    {"SLES52620", 640, 256, 8950, 11230, 390, 147}, // Guncom 2 (E)
    {"SLES51289", 640, 256, 8450, 8900, 456, 164}, // Gunfighter 2 - Jesse James (E)
    {"SLPS25165", 640, 240, 9025, 9800, 390, 138}, // Gunvari Collection (J) (480i)
    {"SCES50889", 640, 256, 9025, 9450, 390, 169}, // Ninja Assault (E)
    {"SLPS20218", 640, 240, 9000, 9200, 320, 134}, // Ninja Assault (J)
    {"SLUS20492", 640, 240, 9025, 9250, 390, 132}, // Ninja Assault (U)
    {"SLES50650", 640, 240, 8475, 9600, 454, 164}, // Resident Evil Survivor 2 (E)
    {"SLES51448", 640, 240, 9025, 9500, 420, 132}, // Resident Evil - Dead Aim (E)
    {"SLUS20669", 640, 240, 9025, 9350, 420, 132}, // Resident Evil - Dead Aim (U)
    {"SLUS20619", 640, 256, 9025, 9175, 453, 154}, // Starsky & Hutch (U)
    {"SCES50300", 640, 256, 9025, 10275, 390, 138}, // Time Crisis II (E)
    {"SLPS20122", 640, 240, 9025, 9750, 390, 154}, // Time Crisis II (J)
    {"SLPS20113", 640, 240, 9025, 9750, 390, 154}, // Time Crisis II (with GunCon 2) (J)
    {"SLUS20219", 640, 240, 9025, 9750, 422, 170}, // Time Crisis 2 (U)
    {"SCES51844", 640, 256, 9025, 10275, 390, 138}, // Time Crisis 3 (E)
    {"SLUS20645", 640, 240, 9025, 9750, 390, 154}, // Time Crisis 3 (U)
    {"SCES52530", 640, 256, 9025, 9900, 390, 153}, // Crisis Zone (E)
    {"SLUS20927", 640, 240, 9025, 9900, 390, 153}, // Time Crisis - Crisis Zone (U) (480i)
    {"SCES50411", 640, 256, 8980, 9990, 421, 138}, // Vampire Night (E)
    {"SLPS25077", 640, 240, 9000, 9750, 422, 118}, // Vampire Night (J)
    {"SLUS20221", 640, 228, 8980, 10250, 422, 124}, // Vampire Night (U)
    {"SLES51229", 512, 256, 11015, 10000, 433, 159}, // Virtua Cop - Elite Edition (E,J) (480i)
    {"SLPM62205", 512, 256, 11015, 10000, 433, 159}, // Virtua Cop Re-Birth (J) (480i)
};

static bool match_serial(const char* serial, const char* name) {
    while (*serial && *name) {
        if (isalnum(*name)) {
            if (toupper(*name) != *serial)
                return false;
            name++;
            serial++;
        }
        else {
            name++;
        }
    }
    return *serial == 0;
}

void set_gun(unsigned port, bool isGun) 
{
    if (port >= MAX_GUNS)
        isGun = false;
    
    port_is_gun[port] = isGun;
}

void load_gun_info(const char* gameName) 
{
    if (gameName != nullptr) 
    {
        for (int i=0; i<sizeof(lightgun_games)/sizeof(*lightgun_games); i++) 
        {
            if (match_serial(lightgun_games[i].serial, gameName)) 
            {
                lightgun_info = &(lightgun_games[i]);
                return;
            }
        }
    }
    lightgun_info = &lightgun_defaults;
}

static void update_gun(CPS2VM* vm, unsigned port) 
{
    // TODO: support more than one gun device
	auto iopOs = dynamic_cast<CIopBios*>(vm->m_iop->m_bios.get());
    auto device = iopOs->GetUsbd()->GetDevice<Iop::CGunCon2UsbDevice>(port);
    if (device == nullptr)
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
        device->SetGunState(buttons,0,0,true);
        return;
    }
    
	int32 screenX = static_cast<int32>(g_input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_SCREEN_X));
	int32 screenY = static_cast<int32>(g_input_state_cb(port, RETRO_DEVICE_LIGHTGUN, 0, RETRO_DEVICE_ID_LIGHTGUN_SCREEN_Y));
    
	int32 x = ( (screenX * lightgun_info->width) / 0x100 * lightgun_info->scale_x + 0x100 * 5000) / (0x100 * 10000)
                    + lightgun_info->center_x;
	int32 y = ( (screenY * lightgun_info->height) / 0x100 * lightgun_info->scale_y + 0x100 * 5000) / (0x100 * 10000)
                    + lightgun_info->center_y;
                    
    device->SetGunState(buttons,x,y,false);
}

void update_guns(CPS2VM* vm) 
{
    for (unsigned i=0; i<MAX_GUNS; i++)
        if (port_is_gun[i])
            update_gun(vm, i);
}