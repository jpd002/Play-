#include "GunCon2Utils.h"
#include "iop/Iop_Usbd.h"
#include "iop/UsbGunCon2Device.h"

static const struct lightgun_info_s lightgun_defaults = { "default__", 640,240,10000,10000,320,120 };

const struct lightgun_info_s* g_lightgun_info = nullptr;

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

bool load_gun_info(const char* gameName) 
{
    if (gameName != nullptr) 
    {
        for (int i=0; i<sizeof(lightgun_games)/sizeof(*lightgun_games); i++) 
        {
            if (match_serial(lightgun_games[i].serial, gameName)) 
            {
                g_lightgun_info = &(lightgun_games[i]);
                return true;
            }
        }
    }
    g_lightgun_info = &lightgun_defaults;
    return false;
}

void register_guncon2(CPS2VM* vm, int instance, bool padMode) 
{
    auto bios = vm->m_iop->m_bios.get();
    auto usbd = dynamic_cast<CIopBios*>(bios)->GetUsbd();
    auto ram = vm->m_iop->m_ram;
    auto device = usbd->GetDevice<Iop::CGunCon2UsbDevice>(instance);
    if (!device) {
        usbd->RegisterDevice(std::make_unique<Iop::CGunCon2UsbDevice>(*dynamic_cast<CIopBios*>(bios), ram, instance));
        device = usbd->GetDevice<Iop::CGunCon2UsbDevice>(instance);
    }
    vm->RegisterGunCon2PadHandler(padMode);
}

void guncon2_set_state(CPS2VM* vm, int instance, uint32 buttons, int32 x, int32 y, bool offscreen)
{
    if (instance<0)
        return;
	auto iopOs = dynamic_cast<CIopBios*>(vm->m_iop->m_bios.get());
    auto device = iopOs->GetUsbd()->GetDevice<Iop::CGunCon2UsbDevice>(instance);
    if (!device)
        return;
    device->SetGunState(buttons,x,y,offscreen);
}

void guncon2_set_position(CPS2VM* vm, int instance, int32 x, int32 y, bool offscreen)
{
    if (instance<0)
        return;
	auto iopOs = dynamic_cast<CIopBios*>(vm->m_iop->m_bios.get());
    auto device = iopOs->GetUsbd()->GetDevice<Iop::CGunCon2UsbDevice>(instance);
    if (!device)
        return;
    device->SetGunPosition(x,y,offscreen);
}

void guncon2_set_button(CPS2VM* vm, int instance, PS2::CControllerInfo::BUTTON button, bool pressed)
{
    if (instance<0)
        return;
	auto iopOs = dynamic_cast<CIopBios*>(vm->m_iop->m_bios.get());
    auto device = iopOs->GetUsbd()->GetDevice<Iop::CGunCon2UsbDevice>(instance);
    if (!device)
        return;
    device->SetButtonState(instance, button, pressed, nullptr);
}

