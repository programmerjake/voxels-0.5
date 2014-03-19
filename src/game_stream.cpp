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
#include "game_stream.h"
#include "block.h"
#include "entity.h"

BlockDescriptorPtr GameLoadStream::readBlockDescriptor()
{
    uint32_t blockIndex = readLimitedU32(0, blocks.size());
    if(blockIndex == blocks.size())
    {
        wstring name = readString();
        BlockDescriptorPtr retval = BlockDescriptor::getBlock(name);
        if(retval == nullptr)
            throw InvalidDataValueException("invalid block name");
        blocks.push_back(retval);
        return retval;
    }
    return blocks[blockIndex];
}

void GameStoreStream::writeBlockDescriptor(BlockDescriptorPtr bd)
{
    assert(bd);
    auto iter = blocks.find(bd->name);
    if(iter == blocks.end())
    {
        writeU32(nextBlockIndex);
        blocks[bd->name] = nextBlockIndex++;
        writeString(bd->name);
    }
    else
        writeU32(get<1>(*iter));
}

EntityDescriptorPtr GameLoadStream::readEntityDescriptor()
{
    uint32_t entityIndex = readLimitedU32(0, entities.size());
    if(entityIndex == entities.size())
    {
        wstring name = readString();
        EntityDescriptorPtr retval = EntityDescriptors.get(name);
        if(retval == nullptr)
            throw InvalidDataValueException("invalid entity name");
        entities.push_back(retval);
        return retval;
    }
    return entities[entityIndex];
}

void GameStoreStream::writeEntityDescriptor(EntityDescriptorPtr ed)
{
    assert(ed);
    auto iter = entities.find(ed->name);
    if(iter == entities.end())
    {
        writeU32(nextEntityIndex);
        entities[ed->name] = nextEntityIndex++;
        writeString(ed->name);
    }
    else
        writeU32(get<1>(*iter));
}
