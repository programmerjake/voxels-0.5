#include "builtin_blocks.h"
#include "texture_atlas.h"
#include "util.h"

void initBuiltinBlocks()
{
    BlockDescriptor::initBlock(StoneBlock::ptr = shared_ptr<BlockDescriptor>(new StoneBlock));
    BlockDescriptor::initBlock(BedrockBlock::ptr = shared_ptr<BlockDescriptor>(new BedrockBlock));
    BlockDescriptor::initBlock(AirBlock::ptr = shared_ptr<BlockDescriptor>(new AirBlock));
}

namespace
{
initializer init1(&initBuiltinBlocks);
}

TextureDescriptor StoneBlock::getFaceTexture(BlockFace) const
{
    return TextureAtlas::Stone.td();
}

TextureDescriptor BedrockBlock::getFaceTexture(BlockFace) const
{
    return TextureAtlas::Bedrock.td();
}


shared_ptr<BlockDescriptor> StoneBlock::ptr, BedrockBlock::ptr, AirBlock::ptr;
