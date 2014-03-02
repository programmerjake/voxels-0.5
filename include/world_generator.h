#ifndef WORLD_GENERATOR_H_INCLUDED
#define WORLD_GENERATOR_H_INCLUDED

#include "world.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <utility>
#include <iostream>

class WorldGeneratorPart;
typedef shared_ptr<WorldGeneratorPart> WorldGeneratorPartPtr;
typedef vector<WorldGeneratorPartPtr>::const_iterator WorldGeneratorPartPtrIterator;

class WorldGeneratorPart : public enable_shared_from_this<WorldGeneratorPart>
{
    const WorldGeneratorPart & operator =(const WorldGeneratorPart &) = delete;
    friend class WorldGeneratorParts_t;
public:
    const wstring name;
    const float precedence;
private:
    static vector<WorldGeneratorPartPtr> * partsList;
    static unordered_map<wstring, WorldGeneratorPartPtr> * partsMap;
    static void init()
    {
        if(partsList == nullptr)
        {
            partsList = new vector<WorldGeneratorPartPtr>();
            partsMap = new unordered_map<wstring, WorldGeneratorPartPtr>();
        }
    }
protected:
    WorldGeneratorPart(const WorldGeneratorPart & rt)
        : enable_shared_from_this(rt), name(rt.name), precedence(rt.precedence)
    {
    }
    static void init(shared_ptr<WorldGeneratorPart> wgp)
    {
        init();
        if(!get<1>(partsMap->insert(make_pair(wgp->name, wgp))))
        {
            cerr << "Error : duplicate world generator part name : \"" << wcsrtombs(wgp->name) << "\"\n";
            exit(1);
        }
        partsList->push_back(wgp);
    }
    WorldGeneratorPart(wstring name, float precedence)
        : name(name), precedence(precedence)
    {
    }
public:
    virtual ~WorldGeneratorPart()
    {
    }
    static constexpr VectorI generateChunkSize = VectorI(16, WorldHeight, 16);
    virtual void run(shared_ptr<World> world, PositionI chunkOrigin) = 0;
    virtual shared_ptr<WorldGeneratorPart> duplicate() = 0;
};

struct WorldGeneratorParts_t final
{
    WorldGeneratorPartPtr get(wstring name) const
    {
        WorldGeneratorPart::init();
        if(WorldGeneratorPart::partsMap->find(name) == WorldGeneratorPart::partsMap->end())
            return nullptr;
        return WorldGeneratorPart::partsMap->at(name);
    }
    WorldGeneratorPartPtrIterator begin() const
    {
        WorldGeneratorPart::init();
        return WorldGeneratorPart::partsList->cbegin();
    }
    WorldGeneratorPartPtrIterator end() const
    {
        WorldGeneratorPart::init();
        return WorldGeneratorPart::partsList->cend();
    }
};

extern const WorldGeneratorParts_t WorldGeneratorParts;

inline WorldGeneratorPartPtrIterator begin(const WorldGeneratorParts_t &)
{
    return WorldGeneratorParts.begin();
}

inline WorldGeneratorPartPtrIterator end(const WorldGeneratorParts_t &)
{
    return WorldGeneratorParts.end();
}

class WorldGenerator final
{
private:
    vector<WorldGeneratorPartPtr> parts;
public:
    WorldGeneratorPartPtrIterator begin() const
    {
        return parts.cbegin();
    }
    WorldGeneratorPartPtrIterator end() const
    {
        return parts.cend();
    }
    void add(WorldGeneratorPartPtr part)
    {
        part = part->duplicate();
        auto i = parts.begin();
        for(; i != parts.end(); i++)
        {
            if(part->precedence > (*i)->precedence)
                break;
        }
        parts.insert(i, part);
    }
    void run(shared_ptr<World> world, PositionI chunkOrigin) const
    {
        for(WorldGeneratorPartPtr part : parts)
        {
            part->run(world, chunkOrigin);
        }
    }
};

#endif // WORLD_GENERATOR_H_INCLUDED
