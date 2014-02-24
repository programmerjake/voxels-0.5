#ifndef RENDER_LAYER_H_INCLUDED
#define RENDER_LAYER_H_INCLUDED

#include <cstdint>
#include "stream.h"

using namespace std;

enum class RenderLayer : uint_fast8_t
{
    Opaque,
    Translucent,
    Last
};

inline void writeRenderLayer(RenderLayer rl, Writer &writer)
{
    writer.writeU8((uint8_t)rl);
}

inline RenderLayer readRenderLayer(Reader &reader)
{
    return (RenderLayer)reader.readLimitedU8(0, (uint8_t)RenderLayer::Last);
}

void renderLayerSetup(RenderLayer rl); // in world.cpp

#endif // RENDER_LAYER_H_INCLUDED
