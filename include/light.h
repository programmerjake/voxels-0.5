#ifndef LIGHT_H_INCLUDED
#define LIGHT_H_INCLUDED

#include <cstdint>
#include "game_stream.h"

using namespace std;

enum class LightProperties : uint8_t
{
    Transparent,
    ScatteringTranslucent,
    NonscatteringTranslucent,
    Water,
    Opaque,
    Last
};

class Lighting final
{
private:
    uint8_t artificialLight, scatteredNaturalLight, directNaturalLight;
    Lighting(uint8_t artificialLight, uint8_t scatteredNaturalLight, uint8_t directNaturalLight)
        : artificialLight(artificialLight), scatteredNaturalLight(scatteredNaturalLight), directNaturalLight(directNaturalLight)
    {
    }
public:
    static constexpr unsigned MAX_INTENSITY = (1 << 4) - 1; // 4 bits
    static Lighting calc(LightProperties p, uint8_t emit, Lighting nx, Lighting px, Lighting ny, Lighting py, Lighting nz, Lighting pz)
    {
        assert(emit <= MAX_INTENSITY);
        Lighting v = py;
        v.artificialLight = max(v.artificialLight, nx.artificialLight);
        v.artificialLight = max(v.artificialLight, px.artificialLight);
        v.artificialLight = max(v.artificialLight, ny.artificialLight);
        v.artificialLight = max(v.artificialLight, nz.artificialLight);
        v.artificialLight = max(v.artificialLight, pz.artificialLight);

        v.scatteredNaturalLight = max(v.scatteredNaturalLight, nx.scatteredNaturalLight);
        v.scatteredNaturalLight = max(v.scatteredNaturalLight, px.scatteredNaturalLight);
        v.scatteredNaturalLight = max(v.scatteredNaturalLight, ny.scatteredNaturalLight);
        v.scatteredNaturalLight = max(v.scatteredNaturalLight, nz.scatteredNaturalLight);
        v.scatteredNaturalLight = max(v.scatteredNaturalLight, pz.scatteredNaturalLight);

        v.artificialLight = max<uint8_t>(1, v.artificialLight) - 1;
        v.scatteredNaturalLight = max<uint8_t>(1, v.scatteredNaturalLight) - 1;
        switch(p)
        {
        case LightProperties::Transparent:
            break;
        case LightProperties::ScatteringTranslucent:
            v.directNaturalLight = 0;
            break;
        case LightProperties::Water:
            v.directNaturalLight = max<uint8_t>(2, v.directNaturalLight) - 2;
            v.artificialLight = max<uint8_t>(1, v.artificialLight) - 1;
            v.scatteredNaturalLight = max<uint8_t>(1, v.scatteredNaturalLight) - 1;
            break;
        case LightProperties::NonscatteringTranslucent:
            v.directNaturalLight = max<uint8_t>(1, v.directNaturalLight) - 1;
            break;
        case LightProperties::Opaque:
            v = Lighting();
            break;
        default:
            assert(false);
        }
        v.artificialLight = max(v.artificialLight, emit);
        v.scatteredNaturalLight = max(v.scatteredNaturalLight, v.directNaturalLight);
        return v;
    }
    Lighting()
        : artificialLight(0), scatteredNaturalLight(0), directNaturalLight(0)
    {
    }
    static Lighting sky()
    {
        return Lighting(0, MAX_INTENSITY, MAX_INTENSITY);
    }
    unsigned calcLight(unsigned naturalBrightness) const
    {
        return max((unsigned)artificialLight, (unsigned)(scatteredNaturalLight * naturalBrightness) / MAX_INTENSITY);
    }
    void write(GameStoreStream & gss) const
    {
        static_assert(MAX_INTENSITY + 1 == 1 << 4, "MAX_INTENSITY must be 1111b"); // must be 4 bits
        uint_fast16_t v = artificialLight;
        v <<= 4;
        v |= scatteredNaturalLight;
        v <<= 4;
        v |= directNaturalLight;
        gss.writeU16(v);
    }
    static Lighting read(GameLoadStream & gls)
    {
        static_assert(MAX_INTENSITY + 1 == 1 << 4, "MAX_INTENSITY must be 1111b"); // must be 4 bits
        uint_fast16_t v = gls.readLimitedU16(0, (1 << 12) - 1);
        return Lighting((v >> 8) & MAX_INTENSITY, (v >> 4) & MAX_INTENSITY, v & MAX_INTENSITY);
    }
};

#endif // LIGHT_H_INCLUDED
