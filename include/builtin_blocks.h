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
public:
    static shared_ptr<BlockDescriptor> ptr;
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
public:
    static shared_ptr<BlockDescriptor> ptr;
};

class AirBlock final : public BlockDescriptor
{
    friend void initBuiltinBlocks();
protected:
    AirBlock()
        : BlockDescriptor(L"builtin.air")
    {
    }
    virtual BlockData loadInternal(GameLoadStream &) const override
    {
        return BlockData(ptr);
    }
    virtual void storeInternal(BlockData, GameStoreStream &) const override
    {
    }
    virtual shared_ptr<RenderObjectBlockMesh> getBlockMesh(BlockIterator) const override
    {
        if(blockMesh)
        {
            return blockMesh;
        }
        return blockMesh = makeBlockMesh();
    }
    virtual void onMove(BlockIterator) const override
    {
    }
private:
    mutable shared_ptr<RenderObjectBlockMesh> blockMesh;
    static shared_ptr<RenderObjectBlockMesh> makeBlockMesh();
public:
    static shared_ptr<BlockDescriptor> ptr;
};

#endif // BUILTIN_BLOCKS_H_INCLUDED
