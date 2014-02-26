#include "stonelikeblock.h"

StoneLikeBlock::StoneLikeBlock(wstring name)
    : Block(name)
{
}

BlockData StoneLikeBlock::loadInternal(GameLoadStream & gls) const
{
    return BlockData(shared_from_this());
}

void StoneLikeBlock::storeInternal(BlockData data, GameStoreStream & gss) const
{
    // empty on purpose
}

shared_ptr<RenderObjectBlockMesh> StoneLikeBlock::getBlockMesh(BlockIterator bi) const
{
    if(blockMesh == nullptr)
        blockMesh = internalMakeBlockMesh();
    return blockMesh;
}

virtual shared_ptr<RenderObjectBlockMesh> internalMakeBlockMesh() const
{
    return make_shared<RenderObjectBlockMesh>(1, LightProperties(LightPropertiesType::Opaque, 0), Mesh(new Mesh_t),
                Generate::unitBox(getFaceTexture(BlockFace::NX), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), getFaceTexture(BlockFace::PX), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureDescriptor(), getFaceTexture(BlockFace::NY), TextureDescriptor(), TextureDescriptor(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), getFaceTexture(BlockFace::PY), TextureDescriptor(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), getFaceTexture(BlockFace::NZ), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), getFaceTexture(BlockFace::PZ)),
                true, true, true, true, true, true, RenderLayer::Opaque
                                                 );
}


