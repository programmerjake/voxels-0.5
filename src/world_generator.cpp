#include "world_generator.h"
#include "builtin_entities.h"

atomic_uint WorldRandom::nextRandomClass(WorldRandom::RandomClassUserStart);
const WorldGeneratorParts_t WorldGeneratorParts;
vector<WorldGeneratorPartConstPtr> *WorldGeneratorPart::partsList = nullptr;
unordered_map<wstring, WorldGeneratorPartConstPtr> *WorldGeneratorPart::partsMap = nullptr;

namespace
{
class TestingDefaultWorldGeneratorPart final : public WorldGeneratorPart
{
public:
    TestingDefaultWorldGeneratorPart()
        : WorldGeneratorPart(L"testing.default", 1e10)
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
                float value = world->random.getFBM2D(PositionF(rpos.x + chunkOrigin.x, 0, rpos.z + chunkOrigin.z, chunkOrigin.d) * 0.05f, VectorF(2), 0.4f, 3,
                                            WorldRandom::RandomClassGround);
                for(rpos.y = 0; rpos.y < generateChunkSize.y; rpos.y++)
                {
                    PositionI pos = rpos + chunkOrigin;
                    bi = pos;

                    if(pos.y == AverageGroundHeight + 10 && rpos.x == 0 && rpos.z == 0)
                        world->addEntity(EntityBlock::make(BlockDescriptors.get(L"builtin.bedrock"), (PositionF)pos + VectorF(0.5)));

                    if(value < 0.2 * (pos.y - AverageGroundHeight))
                    {
                        bi.set(BlockData(BlockDescriptors.get(L"builtin.air")));
                    }
                    else if(pos.y == 0)
                    {
                        bi.set(BlockData(BlockDescriptors.get(L"builtin.bedrock")));
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
        return WorldGeneratorPartPtr(new TestingDefaultWorldGeneratorPart(*this));
    }
    static void sinit()
    {
        init(WorldGeneratorPartPtr(new TestingDefaultWorldGeneratorPart));
    }
};
initializer init1([]()
{
    TestingDefaultWorldGeneratorPart::sinit();
});
}

WorldGenerator WorldGenerator::makeDefault()
{
    WorldGenerator retval;
    retval.add(WorldGeneratorParts.get(L"testing.default"));
    return retval;
}
