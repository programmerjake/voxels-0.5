#include "builtin_blocks.h"
#include "texture_atlas.h"
#include "util.h"

void initBuiltinBlocks()
{
    BlockDescriptor::addToBlocksList(shared_ptr<BlockDescriptor>(new StoneBlock));
    BlockDescriptor::addToBlocksList(shared_ptr<BlockDescriptor>(new BedrockBlock));
    BlockDescriptor::addToBlocksList(shared_ptr<BlockDescriptor>(new AirBlock));
}

namespace
{
initializer init1(&initBuiltinBlocks);
}

TextureDescriptor StoneBlock::getFaceTexture(BlockFace face) const
{
    return TextureAtlas::Stone.td();
}

TextureDescriptor BedrockBlock::getFaceTexture(BlockFace face) const
{
    return TextureAtlas::Bedrock.td();
}
