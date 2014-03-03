#include "world.h"
#ifndef WORLD_GENERATOR_H_INCLUDED
#define WORLD_GENERATOR_H_INCLUDED

#include <memory>
#include <vector>
#include <unordered_map>
#include <utility>
#include <iostream>
#include <random>
#include <cmath>
#include <mutex>

using namespace std;

class WorldRandom final
{
public:
    typedef unsigned RandomClass;
    static constexpr RandomClass RandomClassNull = 0;
    static constexpr RandomClass RandomClassGround = RandomClassNull + 1;
    static constexpr RandomClass RandomClassUserStart = RandomClassGround + 1;
    const uint_fast32_t seed;
private:
    recursive_mutex & lock;
    vector<unordered_map<PositionI, uint_fast32_t>> values;
    static atomic_uint nextRandomClass;
    uint_fast32_t internalRandom(PositionI pos, RandomClass rc) const
    {
        uint64_t v = pos.x;
        v *= 65537;
        v += pos.y;
        v *= 8191;
        v += pos.z;
        v *= 1627;
        v += rc;
        v *= 65537;
        v += seed;
        for(int i = 0; i < 7; i++)
        {
            v = 1 + v * 6364136223846793005;
        }
        return (uint_fast32_t)(v >> 32);
    }
public:
    static RandomClass getNewRandomClass()
    {
        return nextRandomClass++;
    }
    WorldRandom(uint32_t seed, recursive_mutex & lock)
        : seed(seed), lock(lock)
    {
    }
    uint_fast32_t getRandomU32(PositionI pos, RandomClass rc)
    {
        lock_guard<recursive_mutex> lockIt(lock);
        assert(rc != RandomClassNull && rc < nextRandomClass);
        if(values.size() < rc)
            values.resize(rc);
        unordered_map<PositionI, uint_fast32_t> & valuesMap = values[rc - 1];
        auto iter = valuesMap.find(pos);
        if(iter != valuesMap.end())
            return get<1>(*iter);
        return valuesMap[pos] = internalRandom(pos, rc);
    }
    int_fast32_t getRandomS32(PositionI pos, RandomClass rc)
    {
        return getRandomU32(pos, rc);
    }
    float getRandomFloat(PositionI pos, RandomClass rc)
    {
        return (float)(int64_t)(uint64_t)getRandomU32(pos, rc) / (float)((uint64_t)1 << 32);
    }
    float getRandomFloat(PositionF pos, RandomClass rc)
    {
        lock_guard<recursive_mutex> lockIt(lock);
        PositionI nxnynz(pos);
        float tx = pos.x - floor(pos.x);
        float ty = pos.y - floor(pos.y);
        float tz = pos.z - floor(pos.z);
        float vnxnynz = getRandomFloat(nxnynz + VectorI(0, 0, 0), rc);
        float vnxnypz = getRandomFloat(nxnynz + VectorI(0, 0, 1), rc);
        float vnxpynz = getRandomFloat(nxnynz + VectorI(0, 1, 0), rc);
        float vnxpypz = getRandomFloat(nxnynz + VectorI(0, 1, 1), rc);
        float vpxnynz = getRandomFloat(nxnynz + VectorI(1, 0, 0), rc);
        float vpxnypz = getRandomFloat(nxnynz + VectorI(1, 0, 1), rc);
        float vpxpynz = getRandomFloat(nxnynz + VectorI(1, 1, 0), rc);
        float vpxpypz = getRandomFloat(nxnynz + VectorI(1, 1, 1), rc);
        float vnxny = vnxnynz + tz * (vnxnypz - vnxnynz);
        float vnxpy = vnxpynz + tz * (vnxpypz - vnxpynz);
        float vpxny = vpxnynz + tz * (vpxnypz - vpxnynz);
        float vpxpy = vpxpynz + tz * (vpxpypz - vpxpynz);
        float vnx = vnxny + ty * (vnxpy - vnxny);
        float vpx = vpxny + ty * (vpxpy - vpxny);
        return vnx + tx * (vpx - vnx);
    }
    float getRandomFloat2D(PositionF pos, RandomClass rc)
    {
        lock_guard<recursive_mutex> lockIt(lock);
        PositionI nxnz(pos);
        float tx = pos.x - floor(pos.x);
        float tz = pos.z - floor(pos.z);
        float vnxnz = getRandomFloat(nxnz + VectorI(0, 0, 0), rc);
        float vnxpz = getRandomFloat(nxnz + VectorI(0, 0, 1), rc);
        float vpxnz = getRandomFloat(nxnz + VectorI(1, 0, 0), rc);
        float vpxpz = getRandomFloat(nxnz + VectorI(1, 0, 1), rc);
        float vnx = vnxnz + tz * (vnxpz - vnxnz);
        float vpx = vpxnz + tz * (vpxpz - vpxnz);
        return vnx + tx * (vpx - vnx);
    }
    float getFBM(PositionF pos, VectorF scale, float factor, int octaves, RandomClass rc)
    {
        lock_guard<recursive_mutex> lockIt(lock);
        float retval = 0, currentFactor = 1;
        for(int i = 0; i < octaves; i++)
        {
            retval += currentFactor * (2 * getRandomFloat(pos, rc) - 1);
            currentFactor *= factor;
            pos *= scale;
        }
        return retval;
    }
    float getFBM2D(PositionF pos, VectorF scale, float factor, int octaves, RandomClass rc)
    {
        scale.y = 1;
        lock_guard<recursive_mutex> lockIt(lock);
        float retval = 0, currentFactor = 1;
        for(int i = 0; i < octaves; i++)
        {
            retval += currentFactor * (2 * getRandomFloat2D(pos, rc) - 1);
            currentFactor *= factor;
            pos *= scale;
        }
        return retval;
    }
};

class WorldGeneratorPart;
typedef shared_ptr<WorldGeneratorPart> WorldGeneratorPartPtr;
typedef shared_ptr<const WorldGeneratorPart> WorldGeneratorPartConstPtr;
typedef vector<WorldGeneratorPartConstPtr>::const_iterator WorldGeneratorPartPtrIterator;

class WorldGeneratorPart : public enable_shared_from_this<WorldGeneratorPart>
{
    const WorldGeneratorPart & operator =(const WorldGeneratorPart &) = delete;
    friend class WorldGeneratorParts_t;
public:
    const wstring name;
    const float precedence;
private:
    static vector<WorldGeneratorPartConstPtr> * partsList;
    static unordered_map<wstring, WorldGeneratorPartConstPtr> * partsMap;
    static void init()
    {
        if(partsList == nullptr)
        {
            partsList = new vector<WorldGeneratorPartConstPtr>();
            partsMap = new unordered_map<wstring, WorldGeneratorPartConstPtr>();
        }
    }
protected:
    WorldGeneratorPart(const WorldGeneratorPart & rt)
        : enable_shared_from_this(rt), name(rt.name), precedence(rt.precedence)
    {
    }
    static void init(WorldGeneratorPartConstPtr wgp)
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
    static constexpr VectorI generateChunkSizeModMask = generateChunkSize - VectorI(1);
    static constexpr VectorI generateChunkSizeFloorMask = VectorI(~generateChunkSizeModMask.x, ~generateChunkSizeModMask.y, ~generateChunkSizeModMask.z);
    virtual void run(shared_ptr<World> world, PositionI chunkOrigin) = 0;
    virtual WorldGeneratorPartPtr duplicate() const = 0;
};

struct WorldGeneratorParts_t final
{
    WorldGeneratorPartConstPtr get(wstring name) const
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
    void add(WorldGeneratorPartConstPtr part)
    {
        WorldGeneratorPartPtr p = part->duplicate();
        auto i = parts.begin();
        for(; i != parts.end(); i++)
        {
            if(p->precedence < (*i)->precedence)
                break;
        }
        parts.insert(i, p);
    }
    void run(shared_ptr<World> world, PositionI chunkOrigin) const
    {
        for(WorldGeneratorPartPtr part : parts)
        {
            part->run(world, chunkOrigin);
        }
    }
    static WorldGenerator makeDefault();
};

#endif // WORLD_GENERATOR_H_INCLUDED
