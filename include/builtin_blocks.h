/*
 * Voxels is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Voxels is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Voxels; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
#ifndef BUILTIN_BLOCKS_H_INCLUDED
#define BUILTIN_BLOCKS_H_INCLUDED

#include "block.h"
#include "stonelikeblock.h"
#include "gravity_affected_block.h"

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
        : BlockDescriptor(L"builtin.air", LightProperties(LightPropertiesType::Transparent, 0))
    {
    }
    virtual BlockData loadInternal(GameLoadStream &) const override
    {
        return BlockData(ptr);
    }
    virtual void storeInternal(BlockData, GameStoreStream &) const override
    {
    }
public:
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
    shared_ptr<RenderObjectBlockMesh> makeBlockMesh() const;
public:
    static shared_ptr<BlockDescriptor> ptr;
};

class GlassBlock final : public BlockDescriptor
{
    friend void initBuiltinBlocks();
protected:
    GlassBlock()
        : BlockDescriptor(L"builtin.glass", LightProperties(LightPropertiesType::Transparent, 0))
    {
    }
    virtual BlockData loadInternal(GameLoadStream &) const override
    {
        return BlockData(ptr);
    }
    virtual void storeInternal(BlockData, GameStoreStream &) const override
    {
    }
public:
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
        return static_pointer_cast<PhysicsObject>(make_shared<PhysicsBox>((VectorI)pos + VectorF(0.5), VectorF(0.5), VectorF(0), VectorF(0), VectorF(0), pos.d, PhysicsProperties(PhysicsProperties::INFINITE_MASS, 0.8, 0.1)));
    }
private:
    mutable shared_ptr<RenderObjectBlockMesh> blockMesh;
    shared_ptr<RenderObjectBlockMesh> makeBlockMesh() const;
public:
    static shared_ptr<BlockDescriptor> ptr;
};

class SandBlock final : public GravityAffectedBlock
{
    friend void initBuiltinBlocks();
protected:
    SandBlock()
        : GravityAffectedBlock(L"builtin.sand", LightProperties(LightPropertiesType::Opaque, 0))
    {
    }
public:
    virtual ~SandBlock()
    {
    }
protected:
    virtual BlockData loadInternal(GameLoadStream &) const override
    {
        return BlockData(ptr);
    }
    virtual void storeInternal(BlockData, GameStoreStream &) const override
    {
    }
public:
    virtual shared_ptr<RenderObjectBlockMesh> getBlockMesh(BlockIterator) const override
    {
        if(blockMesh)
        {
            return blockMesh;
        }
        return blockMesh = makeBlockMesh();
    }
    virtual Mesh makeBlockEntityMesh() const override;
    virtual shared_ptr<PhysicsObject> getPhysicsObject(PositionI pos) const override
    {
        return static_pointer_cast<PhysicsObject>(make_shared<PhysicsBox>((VectorI)pos + VectorF(0.5), VectorF(0.5), VectorF(0), VectorF(0), VectorF(0), pos.d, PhysicsProperties(PhysicsProperties::INFINITE_MASS, 0.8, 0.1)));
    }
private:
    mutable shared_ptr<RenderObjectBlockMesh> blockMesh;
    shared_ptr<RenderObjectBlockMesh> makeBlockMesh() const;
public:
    static shared_ptr<BlockDescriptor> ptr;
};

#endif // BUILTIN_BLOCKS_H_INCLUDED
