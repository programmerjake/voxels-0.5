#ifndef BLOCK_H_INCLUDED
#define BLOCK_H_INCLUDED

#include <memory>
#include <cstdint>
#include <cwchar>
#include <string>
#include <map>
#include <cstdlib>
#include <iostream>

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
    explicit BlockData(weak_ptr<BlockDescriptor> desc = nullptr, int32_t idata = 0, shared_ptr<ExtraBlockData> extraData = nullptr)
        : desc(desc), idata(idata), extraData(extraData)
    {
    }
};

#ifndef WORLD_H_INCLUDED
class World;
#endif // WORLD_H_INCLUDED

struct BlockDescriptor
{
private:
    static map<wstring, shared_ptr<BlockDescriptor>> *blocks;
    static vector<shared_ptr<BlockDescriptor>> *blocksList;
    static addToBlocksList(shared_ptr<BlockDescriptor> bd) /// call with all constructed BlockDescriptor daughter classes
    {
        if(blocks == nullptr) // so that we don't have problems with static initialization order
        {
            blocks = new map<wstring, shared_ptr<BlockDescriptor>>;
            blocksList = new vector<shared_ptr<BlockDescriptor>>;
        }
        shared_ptr<BlockDescriptor> & b = (*blocks)[bd.name];
        if(b)
        {
            cerr << "Error : duplicate block name : \"" << bd.name << "\"\n";
            exit(1);
        }
        b = bd;
        blocksList->push_back(bd);
    }
public:
    const wstring name;
    static weak_ptr<BlockDescriptor> getBlock(wstring name)
    {
        return blocks->at(name);
    }
protected:
    BlockDescriptor(wstring name)
        : name(name)
    {
    }
    virtual ~BlockDescriptor()
    {
    }
public:
#error finish
};

#include "world.h"

#endif // BLOCK_H_INCLUDED
