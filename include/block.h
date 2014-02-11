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

using namespace std;

struct BlockDescriptor;

class ExtraBlockData
{
public:
    virtual ~ExtraBlockData()
    {
    }
};

struct BlockData
{
    weak_ptr<BlockDescriptor> desc;
    int32_t idata;
    shared_ptr<ExtraBlockData> extraData;
    explicit BlockData(weak_ptr<BlockDescriptor> desc = weak_ptr<BlockDescriptor>(), int32_t idata = 0, shared_ptr<ExtraBlockData> extraData = nullptr)
        : desc(desc), idata(idata), extraData(extraData)
    {
    }
};

struct BlockDescriptor;

typedef shared_ptr<const BlockDescriptor> BlockDescriptorPtr;

#include "world.h"
#include "game_stream.h"

struct BlockDescriptor : public enable_shared_from_this<BlockDescriptor>
{
private:
    static map<wstring, BlockDescriptorPtr> *blocks;
    static vector<BlockDescriptorPtr> *blocksList;
    static void addToBlocksList(BlockDescriptorPtr bd) /// call with all constructed BlockDescriptor daughter classes
    {
        if(blocks == nullptr) // so that we don't have problems with static initialization order
        {
            blocks = new map<wstring, BlockDescriptorPtr>;
            blocksList = new vector<BlockDescriptorPtr>;
        }
        shared_ptr<const BlockDescriptor> & b = (*blocks)[bd->name];
        if(b)
        {
            cerr << "Error : duplicate block name : \"" << bd->name << "\"\n";
            exit(1);
        }
        b = bd;
        blocksList->push_back(bd);
    }
public:
    const wstring name;
    static BlockDescriptorPtr getBlock(wstring name)
    {
        if(blocks->find(name) == blocks->end())
            return nullptr;
        return blocks->at(name);
    }
    BlockDescriptor(wstring name)
        : name(name)
    {
    }
    virtual ~BlockDescriptor()
    {
    }
protected:
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

#endif // BLOCK_H_INCLUDED
