#ifndef ENTITY_BLOCK_H_INCLUDED
#define ENTITY_BLOCK_H_INCLUDED

#include "entity.h"
#include "block.h"

class EntityBlock final : public EntityDescriptor
{
private:
    struct ExtraData final : public ExtraEntityData
    {
        BlockDescriptorPtr block;
        ExtraData(BlockDescriptorPtr block)
            : block(block)
        {
        }
    };
    friend void initEntityBlock();
    EntityBlock()
        : EntityDescriptor(L"builtin.block")
    {
    }
public:
    static EntityData make(BlockDescriptorPtr block, PositionF position, VectorF velocity = VectorF(0))
    {
        assert(block);
        return EntityData(EntityDescriptors.get(L"builtin.block"), position, velocity, shared_ptr<ExtraEntityData>(new ExtraData(block)));
    }
    virtual EntityData loadInternal(GameLoadStream & gls) const override
    {
        BlockDescriptorPtr block = gls.readBlockDescriptor();
        PositionF position;
        position.x = gls.readFiniteF32();
        position.y = gls.readFiniteF32();
        position.z = gls.readFiniteF32();
        position.d = gls.readDimension();
        VectorF velocity;
        velocity.x = gls.readFiniteF32();
        velocity.y = gls.readFiniteF32();
        velocity.z = gls.readFiniteF32();
        return EntityData(shared_from_this(), position, velocity, shared_ptr<ExtraEntityData>(new ExtraData(block)));
    }
    virtual void storeInternal(EntityData data, GameStoreStream & gss) const override
    {
        assert(data.extraData);
        auto eData = dynamic_pointer_cast<ExtraData>(data.extraData);
        assert(eData);
        gss.writeBlockDescriptor(eData->block);
        gss.writeF32(data.position.x);
        gss.writeF32(data.position.y);
        gss.writeF32(data.position.z);
        gss.writeDimension(data.position.d);
        gss.writeF32(data.velocity.x);
        gss.writeF32(data.velocity.y);
        gss.writeF32(data.velocity.z);
    }
public:
    virtual shared_ptr<RenderObjectEntity> getEntity(EntityData & entity, shared_ptr<World> world) const override;
    virtual void onMove(EntityData & entity, shared_ptr<World> world, float deltaTime) const override;
};

#endif // ENTITY_BLOCK_H_INCLUDED
