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
