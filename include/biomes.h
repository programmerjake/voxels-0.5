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
#ifndef BIOMES_H_INCLUDED
#define BIOMES_H_INCLUDED

#include "biome_server.h"

class BiomeDesert final : public BiomeDescriptor
{
    friend void initBiomes();
private:
    static const BiomeDesert * ptr;
    BiomeDesert()
        : BiomeDescriptor(L"Desert", Biome::Desert)
    {
        getRandomClass();
    }
    static WorldRandom::RandomClass getRandomClass()
    {
        static WorldRandom::RandomClass rc = WorldRandom::RandomClassNull;
        if(rc == WorldRandom::RandomClassNull)
        {
            rc = WorldRandom::getNewRandomClass();
        }
        return rc;
    }
public:
    virtual float temperature() const override
    {
        return 1.0f;
    }
    virtual float humidity() const override
    {
        return 0.0f;
    }
    virtual float getMatch(PositionI pos, float temperature, float humidity, WorldRandom &) const override
    {
        if(pos.d != Dimension::Overworld)
            return 0;
        return (1 - temperature) * (1 - humidity);
    }
    virtual BlockData getCover(PositionI, WorldRandom &, int depth) const override
    {
        if(depth > 3)
            return BlockData(BlockDescriptors.get(L"builtin.stone"));
        return BlockData(BlockDescriptors.get(L"builtin.sand"));
    }
    virtual bool isBlockValueHeightDependant() const override
    {
        return false;
    }
    virtual float getBlockValue(PositionI posi, WorldRandom & r) const override
    {
        PositionF pos = (PositionF)posi;
        pos.y = 0;
        pos /= 100.0f;
        float retval = r.getFBM2D(pos, VectorF(2), 0.2f, 4, getRandomClass());
        return sgn(retval) * sqrt(abs(retval)) * 5 + 3;
    }
};

#endif // BIOMES_H_INCLUDED
