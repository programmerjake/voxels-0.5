#include "stonelikeblock.h"
#include "generate.h"

StoneLikeBlock::StoneLikeBlock(wstring name)
    : BlockDescriptor(name)
{
}

BlockData StoneLikeBlock::loadInternal(GameLoadStream &) const
{
    return BlockData(shared_from_this());
}

void StoneLikeBlock::storeInternal(BlockData, GameStoreStream & ) const
{
    // empty on purpose
}

shared_ptr<RenderObjectBlockMesh> StoneLikeBlock::getBlockMesh(BlockIterator ) const
{
    if(blockMesh == nullptr)
        blockMesh = internalMakeBlockMesh();
    return blockMesh;
}

shared_ptr<RenderObjectBlockMesh> StoneLikeBlock::internalMakeBlockMesh() const
{
    return make_shared<RenderObjectBlockMesh>(getStoneClass(), LightProperties(LightPropertiesType::Opaque, 0), Mesh(new Mesh_t),
                Generate::unitBox(getFaceTexture(BlockFace::NX), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), getFaceTexture(BlockFace::PX), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureDescriptor(), getFaceTexture(BlockFace::NY), TextureDescriptor(), TextureDescriptor(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), getFaceTexture(BlockFace::PY), TextureDescriptor(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), getFaceTexture(BlockFace::NZ), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), getFaceTexture(BlockFace::PZ)),
                true, true, true, true, true, true, RenderLayer::Opaque
                                                 );
}


