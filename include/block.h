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
#ifndef BLOCK_H_INCLUDED
#define BLOCK_H_INCLUDED

#error finish changing to new physics engine

#include <memory>
#include <cstdint>
#include <cwchar>
#include <string>
#include <map>
#include <cstdlib>
#include <iostream>
#include <vector>
#include "render_object.h"
#include "physics.h"

using namespace std;

struct BlockDescriptor;

class ExtraBlockData
{
public:
    virtual ~ExtraBlockData()
    {
    }
};

struct BlockDescriptor;

typedef shared_ptr<const BlockDescriptor> BlockDescriptorPtr;
typedef vector<BlockDescriptorPtr>::const_iterator BlockDescriptorPtrIterator;

#include "light.h"

struct BlockData
{
    shared_ptr<const BlockDescriptor> desc;
    int32_t idata;
    Lighting light;
    shared_ptr<ExtraBlockData> extraData;
    explicit BlockData(shared_ptr<const BlockDescriptor> desc = shared_ptr<BlockDescriptor>(), int32_t idata = 0, shared_ptr<ExtraBlockData> extraData = nullptr)
        : desc(desc), idata(idata), light(), extraData(extraData)
    {
    }
    bool good() const
    {
        return desc != nullptr;
    }
};

#include "world.h"
#include "game_stream.h"
#include "util.h"

class BlockDescriptor : public enable_shared_from_this<BlockDescriptor>
{
private:
    static map<wstring, BlockDescriptorPtr> *blocks;
    static vector<BlockDescriptorPtr> *blocksList;
protected:
    static void initBlock(BlockDescriptorPtr bd) /// call with all constructed BlockDescriptor daughter classes
    {
        if(blocks == nullptr) // so that we don't have problems with static initialization order
        {
            blocks = new map<wstring, BlockDescriptorPtr>;
            blocksList = new vector<BlockDescriptorPtr>;
        }
        shared_ptr<const BlockDescriptor> & b = (*blocks)[bd->name];
        if(b)
        {
            cerr << "Error : duplicate block name : \"" << wcsrtombs(bd->name) << "\"\n";
            exit(1);
        }
        b = bd;
        blocksList->push_back(bd);
    }
public:
    const wstring name;
    const LightProperties lightProperties;
    static BlockDescriptorPtr getBlock(wstring name)
    {
        if(blocks == nullptr) // so that we don't have problems with static initialization order
        {
            blocks = new map<wstring, BlockDescriptorPtr>;
            blocksList = new vector<BlockDescriptorPtr>;
        }
        if(blocks->find(name) == blocks->end())
            return nullptr;
        return blocks->at(name);
    }
    static BlockDescriptorPtrIterator blocksBegin()
    {
        if(blocks == nullptr) // so that we don't have problems with static initialization order
        {
            blocks = new map<wstring, BlockDescriptorPtr>;
            blocksList = new vector<BlockDescriptorPtr>;
        }
        return blocksList->cbegin();
    }
    static BlockDescriptorPtrIterator blocksEnd()
    {
        if(blocks == nullptr) // so that we don't have problems with static initialization order
        {
            blocks = new map<wstring, BlockDescriptorPtr>;
            blocksList = new vector<BlockDescriptorPtr>;
        }
        return blocksList->cend();
    }
    virtual ~BlockDescriptor()
    {
    }
    virtual shared_ptr<RenderObjectBlockMesh> getBlockMesh(BlockIterator bi) const = 0;
protected:
    BlockDescriptor(wstring name, LightProperties lightProperties)
        : name(name), lightProperties(lightProperties)
    {
    }
    virtual BlockData loadInternal(GameLoadStream & gls) const = 0;
    virtual void storeInternal(BlockData data, GameStoreStream & gss) const = 0;
public:
    virtual void onMove(BlockIterator bi) const = 0;
    static BlockData load(GameLoadStream & gls)
    {
        return gls.readBlockDescriptor()->loadInternal(gls);
    }

    static void store(BlockData data, GameStoreStream & gss)
    {
        gss.writeBlockDescriptor(data.desc);
        data.desc->storeInternal(data, gss);
    }
    virtual Mesh makeBlockEntityMesh() const = 0;
    virtual shared_ptr<PhysicsObjectConstructor> getPhysicsObjectConstructor() const = 0;
    shared_ptr<PhysicsObject> getPhysicsObject(PositionI pos, shared_ptr<PhysicsWorld> physicsWorld) const
    {
        return getPhysicsObjectConstructor()->make((PositionF)pos, VectorF(0), physicsWorld);
    }
};

struct BlockDescriptors_t final
{
    BlockDescriptorPtr get(wstring name) const
    {
        return BlockDescriptor::getBlock(name);
    }
    BlockDescriptorPtrIterator begin() const
    {
        return BlockDescriptor::blocksBegin();
    }
    BlockDescriptorPtrIterator end() const
    {
        return BlockDescriptor::blocksEnd();
    }
};

extern const BlockDescriptors_t BlockDescriptors;

inline BlockDescriptorPtrIterator begin(const BlockDescriptors_t &)
{
    return BlockDescriptor::blocksBegin();
}

inline BlockDescriptorPtrIterator end(const BlockDescriptors_t &)
{
    return BlockDescriptor::blocksEnd();
}

#endif // BLOCK_H_INCLUDED
