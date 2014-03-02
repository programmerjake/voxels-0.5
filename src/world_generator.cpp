#include "world_generator.h"

const WorldGeneratorParts_t WorldGeneratorParts;
vector<WorldGeneratorPartPtr> * WorldGeneratorPart::partsList = nullptr;
unordered_map<wstring, WorldGeneratorPartPtr> * WorldGeneratorPart::partsMap = nullptr;
