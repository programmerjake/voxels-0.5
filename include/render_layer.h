#ifndef RENDER_LAYER_H_INCLUDED
#define RENDER_LAYER_H_INCLUDED

#include <cstdint>

using namespace std;

enum class RenderLayer : uint_fast32_t
{
    Opaque,
    Translucent,
    Last
};

#endif // RENDER_LAYER_H_INCLUDED
