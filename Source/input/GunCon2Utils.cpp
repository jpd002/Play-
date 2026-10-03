#include "GunCon2Utils.h"
#include "iop/Iop_Usbd.h"
#include "iop/UsbGunCon2Device.h"

static const struct {
    char serial[10];
    struct Iop::LightgunInfo info;
} 
lightgun_games[] = {
    {"SLES50930", {640, 256, .9025f, .9450f, 390, 132}}, // Dino Stalker (E, English)
    {"SLES51095", {640, 256, .9025f, .9450f, 390, 132}}, // Dino Stalker (E, French)
    {"SLES51096", {640, 256, .9025f, .9450f, 390, 132}}, // Dino Stalker (E, German)
    {"SLUS20485", {640, 240, .9025f, .9250f, 390, 132}}, // Dino Stalker (U)
    {"SLUS20389", {640, 240, .8925f, .9350f, 422, 141}}, // Endgame (U)
    {"SLES50936", {512, 256, 1.1200f, 1.0000f, 320, 120}}, // Endgame (E) (Guncon2 needs to be connected to USB port 2)
    {"SLPM65139", {640, 240, .9000f, .9150f, 320, 120}}, // Gun Survivor 3: Dino Crisis (J)
    {"SLES52620", {640, 256, .8950f, 1.1230f, 390, 147}}, // Guncom 2 (E)
    {"SLES51289", {640, 256, .8450f, .8900f, 456, 164}}, // Gunfighter 2 - Jesse James (E)
    {"SLPS25165", {640, 240, .9025f, .9800f, 390, 138}}, // Gunvari Collection (J) (480i)
    {"SCES50889", {640, 256, .9025f, .9450f, 390, 169}}, // Ninja Assault (E)
    {"SLPS20218", {640, 240, .9000f, .9200f, 320, 134}}, // Ninja Assault (J)
    {"SLUS20492", {640, 240, .9025f, .9250f, 390, 132}}, // Ninja Assault (U)
    {"SLES50650", {640, 240, .8475f, 9600, 454, 164}}, // Resident Evil Survivor 2 (E)
    {"SLES51448", {640, 240, .9025f, .9500f, 420, 132}}, // Resident Evil - Dead Aim (E)
    {"SLUS20669", {640, 240, .9025f, .9350f, 420, 132}}, // Resident Evil - Dead Aim (U)
    {"SLUS20619", {640, 256, .9025f, .9175f, 453, 154}}, // Starsky & Hutch (U)
    {"SCES50300", {640, 256, .9025f, 1.0275f, 390, 138}}, // Time Crisis II (E)
    {"SLPS20122", {640, 240, .9025f, .9750f, 390, 154}}, // Time Crisis II (J)
    {"SLPS20113", {640, 240, .9025f, .9750f, 390, 154}}, // Time Crisis II (with GunCon 2) (J)
    {"SLUS20219", {640, 240, .9025f, .9750f, 422, 170}}, // Time Crisis 2 (U)
    {"SCES51844", {640, 256, .9025f, 1.0275f, 390, 138}}, // Time Crisis 3 (E)
    {"SLUS20645", {640, 240, .9025f, .9750f, 390, 154}}, // Time Crisis 3 (U)
    {"SCES52530", {640, 256, .9025f, .9900f, 390, 153}}, // Crisis Zone (E)
    {"SLUS20927", {640, 240, .9025f, .9900f, 390, 153}}, // Time Crisis - Crisis Zone (U) (480i)
    {"SCES50411", {640, 256, .8980f, .9990f, 421, 138}}, // Vampire Night (E)
    {"SLPS25077", {640, 240, .9000f, .9750f, 422, 118}}, // Vampire Night (J)
    {"SLUS20221", {640, 228, .8980f, 1.0250f, 422, 124}}, // Vampire Night (U)
    {"SLES51229", {512, 256, 1.1015f, 1.0000f, 433, 159}}, // Virtua Cop - Elite Edition (E,J) (480i)
    {"SLPM62205", {512, 256, 1.1015f, 1.0000f, 433, 159}}, // Virtua Cop Re-Birth (J) (480i)
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

const struct Iop::LightgunInfo* GetLightgunInfo(const char* gameName) 
{
    if (gameName != nullptr) 
    {
        for (int i=0; i<sizeof(lightgun_games)/sizeof(*lightgun_games); i++) 
        {
            if (match_serial(lightgun_games[i].serial, gameName)) 
            {
                return &(lightgun_games[i].info);
            }
        }
    }
    return nullptr;
}

void GunCon2SetState(CPS2VM* vm, int instance, uint32 buttons, float x, float y)
{
    if (instance<0)
        return;
	auto iopOs = dynamic_cast<CIopBios*>(vm->m_iop->m_bios.get());
    auto device = iopOs->GetUsbd()->GetDevice<Iop::CGunCon2UsbDevice>(instance);
    if (!device)
        return;
    device->SetGunButtons(buttons);
    device->SetScreenPosition(x,y);
}

