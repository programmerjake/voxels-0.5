#include "block.h"

map<wstring, shared_ptr<BlockDescriptor>> *BlockDescriptor::blocks = nullptr;
vector<shared_ptr<BlockDescriptor>> *BlockDescriptor::blocksList = nullptr;
