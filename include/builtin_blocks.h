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
    virtual Mesh makeBlockEntityMesh() const override
    {
        return Mesh(new Mesh_t);
    }
    virtual shared_ptr<PhysicsObject> getPhysicsObject(PositionI) const override
    {
        return static_pointer_cast<PhysicsObject>(make_shared<PhysicsEmpty>());
    }
private:
    mutable shared_ptr<RenderObjectBlockMesh> blockMesh;
    static shared_ptr<RenderObjectBlockMesh> makeBlockMesh();
public:
    static shared_ptr<BlockDescriptor> ptr;
};

class GlassBlock final : public BlockDescriptor
{
    friend void initBuiltinBlocks();
protected:
    GlassBlock()
        : BlockDescriptor(L"builtin.glass")
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
    virtual Mesh makeBlockEntityMesh() const override;
    virtual shared_ptr<PhysicsObject> getPhysicsObject(PositionI pos) const override
    {
        return static_pointer_cast<PhysicsObject>(make_shared<PhysicsBox>((VectorI)pos + VectorF(0.5), VectorF(0.5), pos.d, PhysicsProperties(PhysicsProperties::INFINITE_MASS, 0.8, 0.1)));
    }
private:
    mutable shared_ptr<RenderObjectBlockMesh> blockMesh;
    static shared_ptr<RenderObjectBlockMesh> makeBlockMesh();
public:
    static shared_ptr<BlockDescriptor> ptr;
};

#endif // BUILTIN_BLOCKS_H_INCLUDED
