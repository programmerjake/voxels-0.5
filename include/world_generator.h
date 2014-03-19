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
#include "biome_server.h"

//#define WORLD_RANDOM_USE_CACHING

using namespace std;

class WorldRandom final
{
public:
    typedef unsigned RandomClass;
    static constexpr RandomClass RandomClassNull = 0;
    static constexpr RandomClass RandomClassGround = RandomClassNull + 1;
    static constexpr RandomClass RandomClassBiome = RandomClassGround + 1;
    static constexpr RandomClass RandomClassUserStart = RandomClassBiome + 1;
    const uint_fast32_t seed;
private:
    recursive_mutex & lock;
#ifdef WORLD_RANDOM_USE_CACHING
    vector<unordered_map<PositionI, uint32_t>> values;
#endif
    static atomic_uint nextRandomClass;
    uint32_t internalRandom(PositionI pos, RandomClass rc) const
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
        v ^= 0x123456789ABCDEF;
        for(int i = 0; i < 3; i++)
        {
            v = 1 + v * 6364136223846793005;
        }
        return (uint32_t)(v >> 32);
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
    uint32_t getRandomU32(PositionI pos, RandomClass rc)
    {
#ifdef WORLD_RANDOM_USE_CACHING
        lock_guard<recursive_mutex> lockIt(lock);
        assert(rc != RandomClassNull && rc < nextRandomClass);
        if(values.size() < rc)
            values.resize(rc);
        unordered_map<PositionI, uint32_t> & valuesMap = values[rc - 1];
        auto iter = valuesMap.find(pos);
        if(iter != valuesMap.end())
            return get<1>(*iter);
        return valuesMap[pos] = internalRandom(pos, rc);
#else
        return internalRandom(pos, rc);
#endif
    }
    int32_t getRandomS32(PositionI pos, RandomClass rc)
    {
        return getRandomU32(pos, rc);
    }
    float getRandomFloat(PositionI pos, RandomClass rc)
    {
        return (float)(int64_t)(uint64_t)getRandomU32(pos, rc) / (float)((uint64_t)1 << 32);
    }
    float getRandomFloat(PositionF pos, RandomClass rc)
    {
#ifdef WORLD_RANDOM_USE_CACHING
        lock_guard<recursive_mutex> lockIt(lock);
#endif
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
#ifdef WORLD_RANDOM_USE_CACHING
        lock_guard<recursive_mutex> lockIt(lock);
#endif
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
#ifdef WORLD_RANDOM_USE_CACHING
        lock_guard<recursive_mutex> lockIt(lock);
#endif
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
#ifdef WORLD_RANDOM_USE_CACHING
        lock_guard<recursive_mutex> lockIt(lock);
#endif
        float retval = 0, currentFactor = 1;
        for(int i = 0; i < octaves; i++)
        {
            retval += currentFactor * (2 * getRandomFloat2D(pos, rc) - 1);
            currentFactor *= factor;
            pos *= scale;
        }
        return retval;
    }
    void dump()
    {
        cout << "RandomWorld : seed = " << seed << endl;
    }
private:
    struct BiomeProbabilitiesItem final
    {
        bool valid = false;
        BiomeProbabilities probs;
        BiomeProbabilitiesItem()
        {
        }
    };
    unordered_map<PositionI, BiomeProbabilitiesItem> biomeProbabilitiesMap;
    float getTemperatureInternal(PositionI pos)
    {
        PositionF fPos = (PositionF)pos;
        fPos /= 128;
        fPos.y = 0;
        return getFBM2D(fPos, VectorF(2), 0.2f, 4, RandomClassBiome);
    }
    float getHumidityInternal(PositionI pos)
    {
        PositionF fPos = (PositionF)pos;
        fPos /= 128;
        fPos.y = 1;
        return getFBM2D(fPos, VectorF(2), 0.2f, 4, RandomClassBiome);
    }
public:
    BiomeProbabilities getBiomeProbabilities(PositionI pos);
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

class BiomeDescriptor;
typedef const BiomeDescriptor * BiomeDescriptorPtr;

class BiomeDescriptor
{
    BiomeDescriptor(const BiomeDescriptor &) = delete;
    const BiomeDescriptor & operator =(const BiomeDescriptor &) = delete;
public:
    const wstring name;
    const Biome biome;
protected:
    static BiomeDescriptorPtr biomeDescriptors[(int)Biome::Last];
    BiomeDescriptor(wstring name, Biome biome)
        : name(name), biome(biome)
    {
        assert((unsigned)biome < (unsigned)Biome::Last);
        assert(biomeDescriptors[(unsigned)biome] == nullptr);
        biomeDescriptors[(unsigned)biome] = this;
    }
public:
    virtual float temperature() const = 0;
    virtual float humidity() const = 0;
    virtual float getMatch(PositionI pos, float temperature, float humidity, WorldRandom & r) const = 0;
    virtual BlockData getCover(PositionI pos, WorldRandom & r, int depth) const = 0;
    virtual bool isBlockValueHeightDependant() const = 0;
    virtual float getBlockValue(PositionI pos, WorldRandom & r) const = 0;
    static BiomeDescriptorPtr get(Biome biome)
    {
        auto index = (unsigned)biome;
        if(index >= (unsigned)Biome::Last)
            throw range_error("invalid biome in BiomeDescriptor::get");
        return biomeDescriptors[index];
    }
};

inline BiomeProbabilities WorldRandom::getBiomeProbabilities(PositionI pos)
{
    pos.y = 0;
    lock_guard<recursive_mutex> lockIt(lock);
    BiomeProbabilitiesItem & biomeProbabilitiesItem = biomeProbabilitiesMap[pos];
    BiomeProbabilities & retval = biomeProbabilitiesItem.probs;
    if(biomeProbabilitiesItem.valid)
        return retval;
    biomeProbabilitiesItem.valid = true;
    float temperature = getTemperatureInternal(pos);
    float humidity = getHumidityInternal(pos);
    float sum = 0;
    for(size_t i = 0; i < retval.size(); i++)
    {
        BiomeDescriptorPtr pbd = BiomeDescriptor::get((Biome)i);
        if(pbd != nullptr)
            retval[i] = pbd->getMatch(pos, temperature, humidity, *this);
        else
        {
            static bool didWarn = false;
            if(!didWarn)
            {
                didWarn = true;
                cout << "Warning : Not all BiomeDescriptor's are implemented" << endl;
            }
            retval[i] = 0;
        }
        sum += retval[i];
    }
    float sum2 = 0;
    for(float & p : retval)
    {
        p /= sum;
        p *= p;
        p *= p;
        p *= p;
        p *= p;
        p *= p;
        sum2 += p;
    }
    for(float & p : retval)
    {
        p /= sum2;
    }
    return retval;
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
