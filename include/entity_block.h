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
#ifndef ENTITY_BLOCK_H_INCLUDED
#define ENTITY_BLOCK_H_INCLUDED

#include "entity.h"
#include "block.h"

#error finish changing to new physics engine

class EntityBlock final : public EntityDescriptor
{
private:
    static constexpr const VectorF physicsExtents() {return VectorF(0.125);}
    static constexpr const PhysicsProperties physicsProperties() {return PhysicsProperties();}
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
        return EntityData(EntityDescriptors.get(L"builtin.block"), position, velocity, gravityVector, shared_ptr<ExtraEntityData>(new ExtraData(block)));
    }
    virtual ~EntityBlock()
    {
    }
protected:
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
        VectorF acceleration;
        acceleration.x = gls.readFiniteF32();
        acceleration.y = gls.readFiniteF32();
        acceleration.z = gls.readFiniteF32();
        VectorF deltaAcceleration;
        deltaAcceleration.x = gls.readFiniteF32();
        deltaAcceleration.y = gls.readFiniteF32();
        deltaAcceleration.z = gls.readFiniteF32();
        return EntityData(shared_from_this(), position, velocity, acceleration, shared_ptr<ExtraEntityData>(new ExtraData(block)), deltaAcceleration);
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
        gss.writeF32(data.acceleration.x);
        gss.writeF32(data.acceleration.y);
        gss.writeF32(data.acceleration.z);
        gss.writeF32(data.deltaAcceleration.x);
        gss.writeF32(data.deltaAcceleration.y);
        gss.writeF32(data.deltaAcceleration.z);
    }
public:
    virtual shared_ptr<RenderObjectEntity> getEntity(EntityData & entity, shared_ptr<World> world) const override;
    virtual void onMove(EntityData & entity, shared_ptr<World> world, float deltaTime) const override;
    virtual shared_ptr<PhysicsObjectConstructor> getPhysicsObjectConstructor(EntityData &) const override
    {
        return PhysicsObjectConstructor::boxMaker(physicsExtents(), true, false, physicsProperties(), vector<PhysicsConstraint>());
    }
};

#endif // ENTITY_BLOCK_H_INCLUDED
