#include "world.h"
#include "platform.h"
#include "render_layer.h"
#include "builtin_blocks.h"
#include <memory>

using namespace std;

void renderLayerSetup(RenderLayer rl)
{
    switch(rl)
    {
    case RenderLayer::Opaque:
    case RenderLayer::Last:
        glDepthMask(GL_TRUE);
        break;

    case RenderLayer::Translucent:
        glDepthMask(GL_FALSE);
        break;

    default:
        break;
    }
}

BlockData BlockIterator::makeBedrock()
{
    return BlockData(BedrockBlock::ptr);
}

BlockData BlockIterator::makeLitAir()
{
    BlockData retval(AirBlock::ptr);
    retval.light = Lighting::sky();
    return retval;
}

