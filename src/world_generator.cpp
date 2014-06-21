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
#include "world_generator.h"
#include "builtin_entities.h"

#error finish changing to new physics engine

atomic_uint WorldRandom::nextRandomClass(WorldRandom::RandomClassUserStart);
const WorldGeneratorParts_t WorldGeneratorParts;
vector<WorldGeneratorPartConstPtr> *WorldGeneratorPart::partsList = nullptr;
unordered_map<wstring, WorldGeneratorPartConstPtr> *WorldGeneratorPart::partsMap = nullptr;
BiomeDescriptorPtr BiomeDescriptor::biomeDescriptors[(int)Biome::Last] = {nullptr};

namespace
{
class LandGenerator final : public WorldGeneratorPart
{
public:
    LandGenerator()
        : WorldGeneratorPart(L"builtin.land_generator", 0)
    {
    }
    virtual void run(shared_ptr<World> world, PositionI chunkOrigin) override
    {
        lock_guard<recursive_mutex> lockIt(world->lock);
        VectorI rpos;
        BlockIterator bi = world->get(chunkOrigin);

        for(rpos.x = 0; rpos.x < generateChunkSize.x; rpos.x++)
        {
            for(rpos.z = 0; rpos.z < generateChunkSize.z; rpos.z++)
            {
                rpos.y = 0;
                BiomeProbabilities bProbs = world->random.getBiomeProbabilities(rpos + chunkOrigin);
                bool valueIsHeightDependant = false;
                for(size_t i = 0; i < bProbs.size(); i++)
                {
                    if(bProbs[i] < eps)
                        continue;
                    BiomeDescriptorPtr pBiome = BiomeDescriptor::get((Biome)i);
                    assert(pBiome);
                    if(pBiome->isBlockValueHeightDependant())
                    {
                        valueIsHeightDependant = true;
                        break;
                    }
                }
                float value = 0;
                for(rpos.y = 0; rpos.y < generateChunkSize.y; rpos.y++)
                {
                    PositionI pos = rpos + chunkOrigin;
                    if(valueIsHeightDependant || rpos.y == 0)
                    {
                        value = 0;
                        for(size_t i = 0; i < bProbs.size(); i++)
                        {
                            if(bProbs[i] < eps)
                                continue;
                            BiomeDescriptorPtr pBiome = BiomeDescriptor::get((Biome)i);
                            assert(pBiome);
                            value += bProbs[i] * pBiome->getBlockValue(pos, world->random);
                        }
                    }
                    bi = pos;

                    if(value < pos.y - AverageGroundHeight)
                    {
                        bi.set(BlockData(BlockDescriptors.get(L"builtin.air")));
                    }
                    else
                    {
                        bi.set(BlockData(BlockDescriptors.get(L"builtin.stone")));
                    }
                }
            }
        }
    }
    virtual WorldGeneratorPartPtr duplicate() const override
    {
        return WorldGeneratorPartPtr(new LandGenerator(*this));
    }
    static void sinit()
    {
        init(WorldGeneratorPartPtr(new LandGenerator));
    }
};
class CoverGenerator final : public WorldGeneratorPart
{
public:
    CoverGenerator()
        : WorldGeneratorPart(L"builtin.cover_generator", 1)
    {
    }
    virtual void run(shared_ptr<World> world, PositionI chunkOrigin) override
    {
        lock_guard<recursive_mutex> lockIt(world->lock);
        VectorI rpos;
        BlockIterator bi = world->get(chunkOrigin);

        for(rpos.x = 0; rpos.x < generateChunkSize.x; rpos.x++)
        {
            for(rpos.z = 0; rpos.z < generateChunkSize.z; rpos.z++)
            {
                rpos.y = 0;
                BiomeDescriptorPtr pBiome = BiomeDescriptor::get(getCurrentBiome(world->random.getBiomeProbabilities(rpos + chunkOrigin)));
                int depth = 0;
                for(rpos.y = generateChunkSize.y - 1; rpos.y >= 0; rpos.y--, depth++)
                {
                    PositionI pos = rpos + chunkOrigin;
                    bi = pos;
                    if(bi.get().desc->name != L"builtin.stone")
                    {
                        depth = 0;
                        continue;
                    }
                    bi.set(pBiome->getCover(pos, world->random, depth));
                }
            }
        }
    }
    virtual WorldGeneratorPartPtr duplicate() const override
    {
        return WorldGeneratorPartPtr(new CoverGenerator(*this));
    }
    static void sinit()
    {
        init(WorldGeneratorPartPtr(new CoverGenerator));
    }
};
class BasicLightGenerator final : public WorldGeneratorPart
{
public:
    BasicLightGenerator()
        : WorldGeneratorPart(L"builtin.basic_light_generator", 1e10)
    {
    }
    virtual void run(shared_ptr<World> world, PositionI chunkOrigin) override
    {
        lock_guard<recursive_mutex> lockIt(world->lock);
        VectorI rpos;
        BlockIterator bi = world->get(chunkOrigin);

        for(rpos.x = 0; rpos.x < generateChunkSize.x; rpos.x++)
        {
            for(rpos.z = 0; rpos.z < generateChunkSize.z; rpos.z++)
            {
                Lighting curLight = Lighting::sky();
                for(rpos.y = generateChunkSize.y - 1; rpos.y >= 0; rpos.y--)
                {
                    PositionI pos = rpos + chunkOrigin;
                    bi = pos;
                    BlockData bd = bi.get();
                    curLight = Lighting::calc(bd.desc->lightProperties, Lighting(), Lighting(), Lighting(), curLight, Lighting(), Lighting());
                    bd.light = curLight;
                    bi.set(bd);
                }
            }
        }
    }
    virtual WorldGeneratorPartPtr duplicate() const override
    {
        return WorldGeneratorPartPtr(new BasicLightGenerator(*this));
    }
    static void sinit()
    {
        init(WorldGeneratorPartPtr(new BasicLightGenerator));
    }
};
initializer init1([]()
{
    LandGenerator::sinit();
    CoverGenerator::sinit();
    BasicLightGenerator::sinit();
});
}

WorldGenerator WorldGenerator::makeDefault()
{
    WorldGenerator retval;
    retval.add(WorldGeneratorParts.get(L"builtin.land_generator"));
    retval.add(WorldGeneratorParts.get(L"builtin.cover_generator"));
    retval.add(WorldGeneratorParts.get(L"builtin.basic_light_generator"));
    return retval;
}
