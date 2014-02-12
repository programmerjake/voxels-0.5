#include "world.h"
#include "platform.h"
#include "render_layer.h"
#include <memory>

using namespace std;

namespace
{
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
}

BlockIterator::BlockIterator(shared_ptr<World> w, PositionI pos)
    : worldInternal(w), pos(pos)
{
    int ChunkX = ChunkFloorSizeMask & pos.x;
    int ChunkZ = ChunkFloorSizeMask & pos.z;
    chunk = w->getChunk(ChunkPosition(pos));
    chunkIndex = (pos.x & ChunkModSizeMask) + ChunkSize * (pos.y) + ChunkSize * ChunkHeight * (pos.z & ChunkModSizeMask);
}

