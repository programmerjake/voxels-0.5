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
#ifndef PLAYER_H_INCLUDED
#define PLAYER_H_INCLUDED

#include "entity.h"
#include "client.h"
#include "render_object.h"

class EntityPlayer final : public EntityDescriptor
{
private:
    static constexpr VectorF physicsExtents = VectorF(0.3, 0.9, 0.3);
    static constexpr PhysicsProperties physicsProperties(0.1, 0.5, 0.1);
    static constexpr VectorF physicsOffset = VectorF(0, 1.62 - physicsExtents.y, 0);
    struct ExtraData final : public ExtraEntityData
    {
        Client * pclient;
        const wstring name;
        float theta = 0, phi = 0;
        int playerIndex;
        bool flying = false;
        ExtraData(Client * pclient, wstring name, int playerIndex)
            : pclient(pclient), name(name), playerIndex(playerIndex)
        {
        }
    };
    static void init()
    {
#if 1
        initEntity(EntityDescriptorPtr(new EntityPlayer));
#else
#warning finish
#endif
    }
    friend void initEntityPlayer();
    EntityPlayer()
        : EntityDescriptor(L"builtin.player")
    {
    }
public:
    static shared_ptr<EntityData> get(Client & client)
    {
        shared_ptr<EntityData> retval = client.getPropertyPtr<EntityData, 0>(Client::DataType::Player);
        if(!retval->good())
        {
            wstring name = L"";
            static int playerIndex = 0;
            cout << "Server : made player #" << ++playerIndex << endl;
#warning finish
            *retval = EntityData(EntityDescriptors.get(L"builtin.player"), PositionF(0.5, AverageGroundHeight + 0.5, 0.5,
                                        Dimension::Overworld), VectorF(0), VectorF(0), shared_ptr<ExtraEntityData>(new ExtraData(&client, name, playerIndex)));
        }
        return retval;
    }
    static void update(shared_ptr<EntityData> player, PositionF position, VectorF velocity, float theta, float phi, bool flying)
    {
        EntityData & data = *player;
        assert(data.extraData);
        auto eData = dynamic_pointer_cast<ExtraData>(data.extraData);
        assert(eData);
        eData->theta = theta;
        eData->phi = phi;
        eData->flying = flying;
        data.position = position;
        data.velocity = velocity;
        cout << "updated Player #" << eData->playerIndex << "\r" << flush;
    }
protected:
    virtual EntityData loadInternal(GameLoadStream & gls) const override
    {
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
        wstring name = gls.readString();
        return EntityData(shared_from_this(), position, velocity, acceleration, shared_ptr<ExtraEntityData>(new ExtraData(nullptr, name, 0)), deltaAcceleration);
    }
    virtual void storeInternal(EntityData data, GameStoreStream & gss) const override
    {
        assert(data.extraData);
        auto eData = dynamic_pointer_cast<ExtraData>(data.extraData);
        assert(eData);
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
        gss.writeString(eData->name);
    }
public:
    virtual shared_ptr<RenderObjectEntity> getEntity(EntityData & entity, shared_ptr<World> world) const override;
    virtual void onMove(EntityData & entity, shared_ptr<World> world, float deltaTime) const override;
    virtual shared_ptr<PhysicsObjectConstructor> getPhysicsObjectConstructor(EntityData & entity) const override
    {
        return PhysicsObjectConstructor::box(physicsProperties, physicsExtents, physicsOffset);
    }
};

#endif // PLAYER_H_INCLUDED
