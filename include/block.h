#ifndef BLOCK_H_INCLUDED
#define BLOCK_H_INCLUDED

#include <memory>
#include <cstdint>
#include <cwchar>
#include <string>
#include <map>
#include <cstdlib>
#include <iostream>
#include <vector>
#include "render_object.h"

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

struct BlockDescriptor : public enable_shared_from_this<BlockDescriptor>
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
    BlockDescriptor(wstring name)
        : name(name)
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
