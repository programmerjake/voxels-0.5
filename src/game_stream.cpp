#include "game_load_stream.h"
#include "block.h"

BlockDescriptorPtr GameLoadStream::readBlockDescriptor()
{
    uint32_t blockIndex = readLimitedU32(0, blocks.size());
    if(blockIndex == blocks.size())
    {
        wstring name = readString();
        BlockDescriptorPtr retval = BlockDescriptor::getBlock(name);
        if(retval == nullptr)
            throw new InvalidDataValueException("invalid block name");
        blocks.push_back(retval);
        return retval;
    }
    return blocks[blockIndex];
}

void GameStoreStream::writeBlockDescriptor(BlockDescriptorPtr bd)
{
    assert(bd);
    auto iter = blocks.find(bd->name());
    if(iter == blocks.end())
    {
        writeU32(nextBlockIndex);
        blocks[bd->name()] = nextBlockIndex++;
        writeString(bd->name());
    }
    else
        writeU32(get<1>(*iter));
}
