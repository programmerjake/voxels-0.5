#include "block.h"

map<wstring, shared_ptr<const BlockDescriptor>> *BlockDescriptor::blocks = nullptr;
vector<shared_ptr<const BlockDescriptor>> *BlockDescriptor::blocksList = nullptr;
const BlockDescriptors_t BlockDescriptors;
