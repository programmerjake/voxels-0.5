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
#ifndef BIOME_SERVER_H_INCLUDED
#define BIOME_SERVER_H_INCLUDED
#error finish changing to new physics engine

#include <array>

using namespace std;

enum class Biome
{
    Ocean,
    Plains,
    Desert,
    ExtremeHills,
    Forest,
    Taiga,
    Swampland,
    River,
    Nether,
    End,
    FrozenOcean,
    FrozenRiver,
    IcePlains,
    IceMountains,
    MushroomIsland,
    MushroomIslandShore,
    Beach,
    ExtremeHillsEdge,
    Jungle,
    JungleEdge,
    DeepOcean,
    StoneBeach,
    ColdBeach,
    ColdTaiga,
    Savanna,
    Last
};

typedef array<float, (size_t)Biome::Last> BiomeProbabilities;

inline Biome getCurrentBiome(const BiomeProbabilities & probs)
{
    Biome retval = (Biome)0;
    float prob = probs[0];
    for(size_t i = 1; i < probs.size(); i++)
    {
        if(probs[i] > prob)
        {
            prob = probs[i];
            retval = (Biome)i;
        }
    }
    return retval;
}

#endif // BIOME_SERVER_H_INCLUDED
