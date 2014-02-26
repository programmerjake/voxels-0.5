#ifndef BUILTIN_BLOCKS_H_INCLUDED
#define BUILTIN_BLOCKS_H_INCLUDED

#include "block.h"
#include "stonelikeblock.h"

class StoneBlock final : public StoneLikeBlock
{
    friend void initBuiltinBlocks();
protected:
    StoneBlock()
        : StoneLikeBlock(L"builtin.stone")
    {
    }
    virtual TextureDescriptor getFaceTexture(BlockFace face) const override;
};

class BedrockBlock final : public StoneLikeBlock
{
    friend void initBuiltinBlocks();
protected:
    BedrockBlock()
        : StoneLikeBlock(L"builtin.bedrock")
    {
    }
    virtual TextureDescriptor getFaceTexture(BlockFace face) const override;
};

class AirBlock final : public BlockDescriptor
{
    friend void initBuiltinBlocks();
protected:
    AirBlock()
        : BlockDescriptor(L"builtin.air")
    {
    }
    virtual BlockData loadInternal(GameLoadStream &gls) const override
    {
    }
    virtual void storeInternal(BlockData data, GameStoreStream &gss) const override
    {
    }
    virtual shared_ptr<RenderObjectBlockMesh> getBlockMesh(BlockIterator bi) const override
    {
        if(blockMesh)
        {
            return blockMesh;
        }
        return blockMesh = make_shared<RenderObjectBlockMesh>(getRenderObjectBlockClass(), LightProperties(LightPropertiesType::Transparent, 0), Mesh(new Mesh_t), Mesh(new Mesh_t), Mesh(new Mesh_t), Mesh(new Mesh_t), Mesh(new Mesh_t), Mesh(new Mesh_t), Mesh(new Mesh_t), false, false, false, false, false, false, RenderLayer::Opaque);
    }
    virtual void onMove(BlockIterator bi) const override
    {
    }
private:
    mutable shared_ptr<RenderObjectBlockMesh> blockMesh;
};

#endif // BUILTIN_BLOCKS_H_INCLUDED
