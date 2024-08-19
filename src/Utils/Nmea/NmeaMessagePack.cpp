#include <Utils/Nmea/NmeaMessagePack.h>
#include <Utils/Nmea/Nmea.h>

using namespace Ilvo::Utils::Nmea;
using namespace std;

NmeaMessagePack::NmeaMessagePack(const NmeaMessagePack& other) {
    gga = make_shared<NmeaLine>(*other.gga);
    vtg = make_shared<NmeaLine>(*other.vtg);
    hrp = make_shared<NmeaLine>(*other.hrp);
    hdt = make_shared<NmeaLine>(*other.hdt);
}

void NmeaMessagePack::addNmeaLine(shared_ptr<NmeaLine> line)
{
    switch (line->getType())
    {
    case NmeaMessageType::GGA:
        gga = line;
        updated_gga = true;
        break;
    case NmeaMessageType::VTG:
        vtg = line;
        updated_vtg = true;
        break;   
    case NmeaMessageType::HDT:
        hdt = line;
        updated_hdt = true;
        break; 
    case NmeaMessageType::HRP:
        hrp = line;
        updated_hrp = true;
        break;  
    default:
        break;
    }
}

bool NmeaMessagePack::isUpdated()
{
    return updated_gga && updated_hrp && updated_vtg && updated_hdt;
}

void NmeaMessagePack::reset()
{
    updated_gga = false;
    updated_hdt = false;
    updated_hrp = false;
    updated_vtg = false;
}