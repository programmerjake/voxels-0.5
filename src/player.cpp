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
#include "player.h"
#include "generate.h"
#include "server.h"

#error finish converting to use PhysicsObjectConstructor and to use physics offsets

void initEntityPlayer()
{
    EntityPlayer::init();
}

namespace
{
initializer init1(initEntityPlayer);
const wstring program1 = L"io.transform = make_translate(<-0.5, -0.5, -0.5>) ~ make_scale(0.6) ~ make_rotatex(io.phi) ~ make_rotatey(-io.theta) ~ make_translate(io.position); io.doDraw = io.isCurrentPlayer";
const wstring program2 = L"io.transform = make_translate(<-0.5, -0.5, -0.5>) ~ make_scale(0.6) ~ make_rotatex(io.phi) ~ make_rotatey(-io.theta) ~ make_translate(io.position); io.doDraw = not io.isCurrentPlayer";
}

#warning finish

shared_ptr<RenderObjectEntity> EntityPlayer::getEntity(EntityData & data, shared_ptr<World>) const
{
    assert(data.extraData);
    auto eData = dynamic_pointer_cast<ExtraData>(data.extraData);
    assert(eData);
    if(data.entity == nullptr)
    {
        BlockDescriptorPtr block = BlockDescriptors.get(L"builtin.glass");
        shared_ptr<RenderObjectEntityMesh> mesh;
        mesh = make_shared<RenderObjectEntityMesh>(VectorF(0), VectorF(0), true);
        Mesh blockMesh = block->makeBlockEntityMesh();
        mesh->addPart(invert(blockMesh), Script::parse(program1));
        mesh->addPart(blockMesh, Script::parse(program2));
        data.entity = make_shared<RenderObjectEntity>(mesh, data.position, data.velocity, data.acceleration, data.deltaAcceleration, 0);
    }
    data.entity->scriptIOObject->value[L"theta"] = make_shared<Scripting::DataFloat>(eData->theta);
    data.entity->scriptIOObject->value[L"phi"] = make_shared<Scripting::DataFloat>(eData->phi);
    data.entity->scriptIOObject->value[L"isCurrentPlayer"] = make_shared<Scripting::DataBoolean>(false);
    data.entity->position = data.position;
    data.entity->velocity = data.velocity;
    data.entity->acceleration = data.acceleration;
    data.entity->deltaAcceleration = data.deltaAcceleration;
    data.entity->age = 0;
    return data.entity;
}

void EntityPlayer::onMove(EntityData & data, shared_ptr<World> world, float deltaTime) const
{
    getEntity(data, world);
    assert(data.extraData);
    auto eData = dynamic_pointer_cast<ExtraData>(data.extraData);
    assert(eData);
    data.deltaAcceleration = VectorF(0);
    data.acceleration = gravityVector;
    if(eData->flying)
        data.acceleration = VectorF(0);
    int count = (iceil(deltaTime * abs(data.velocity) / 0.5 + 1),1);
    deltaTime /= count;
    BlockIterator bi = world->get((PositionI)data.position);
    data.entity->acceleration = data.acceleration;
    data.entity->deltaAcceleration = data.deltaAcceleration;
    auto pphysicsObject = make_shared<PhysicsBox>((VectorF)data.position + physicsOffset, physicsExtents, data.velocity, data.entity->acceleration, data.entity->deltaAcceleration, data.position.d, PhysicsProperties(0.1, 0.5, 0.1), -physicsOffset);
    PhysicsBox & physicsObject = *pphysicsObject;
    for(int step = 0; step < count; step++)
    {
        data.entity->age += deltaTime;
        int zeroCount = 0;
        while(deltaTime * deltaTime * absSquared(data.velocity) > eps * eps)
        {
            bool supported = false;
            PhysicsCollision firstCollision(data.position + deltaTime * data.velocity + deltaTime * deltaTime * 0.5f * data.entity->acceleration + deltaTime * deltaTime * deltaTime * (1 / 6.0f) * data.entity->deltaAcceleration, data.velocity + deltaTime * data.entity->acceleration + deltaTime * deltaTime * 0.5f * data.entity->deltaAcceleration, VectorF(0), deltaTime);
            physicsObject.reInit((VectorF)data.position + physicsOffset, physicsExtents, data.velocity, data.entity->acceleration, data.entity->deltaAcceleration);
            for(int dx = -1; dx <= 1; dx++)
            {
                for(int dy = -2; dy <= 2; dy++)
                {
                    for(int dz = -1; dz <= 1; dz++)
                    {
                        BlockIterator curBI = bi;
                        curBI += VectorI(dx, dy, dz);
                        shared_ptr<PhysicsObject> otherObject;
                        if(curBI.get().good())
                            otherObject = curBI.get().desc->getPhysicsObject(curBI.position());
                        else
                            otherObject = static_pointer_cast<PhysicsObject>(make_shared<PhysicsBox>((VectorI)curBI.position() + VectorF(0.5), VectorF(0.5), VectorF(0), VectorF(0), VectorF(0), curBI.position().d, PhysicsProperties(PhysicsProperties::INFINITE_MASS, 1, 0)));
                        assert(otherObject);
                        {
                            bool filled = false;
                            float newY;
                            switch(otherObject->type())
                            {
                            case PhysicsObject::Type::Box:
                            {
                                const PhysicsBox * pbox = dynamic_cast<const PhysicsBox *>(otherObject.get());
                                VectorF min = pbox->center - pbox->extents;
                                VectorF max = pbox->center + pbox->extents;
                                if(min.x <= curBI.position().x && max.x >= curBI.position().x + 1 &&
                                   min.y <= curBI.position().y && max.y >= curBI.position().y + 1 &&
                                   min.z <= curBI.position().z && max.z >= curBI.position().z + 1)
                                {
                                    newY = max.y + physicsObject.extents.y - physicsOffset.y;
                                    filled = true;
                                }
                                VectorF temp;
                                if(isBoxCollision(pbox->center, pbox->extents, physicsObject.center - VectorF(0, eps * 10, 0) + physicsOffset, physicsObject.extents, temp) && !isBoxCollision(pbox->center, pbox->extents, physicsObject.center + physicsOffset, physicsObject.extents, temp))
                                {
                                    supported = true;
                                }
                                break;
                            }
                            case PhysicsObject::Type::None:
                                break;
                            }
                            if(filled && zeroCount >= 2 && dx == 0 && dy == 0 && dz == 0)
                            {
                                firstCollision.time = 0;
                                firstCollision.newPosition = data.position;
                                firstCollision.newPosition.y = newY;
                                firstCollision.newVelocity = VectorF(0);
                                firstCollision.collisionNormal = VectorF(0, 1, 0);
                                break;
                            }
                        }
                        PhysicsCollision collision = physicsObject.collide(otherObject, deltaTime);
                        if(collision.valid)
                        {
                            if(collision.time < eps)
                            {
                                if(zeroCount > 25)
                                    collision.valid = false;
                                else
                                    zeroCount++;
                            }
                            else
                                zeroCount = 0;
                        }
                        if(collision.valid && collision.time < firstCollision.time)
                            firstCollision = collision;
                    }
                }
            }
            deltaTime -= firstCollision.time;
            data.setPosition(firstCollision.newPosition + eps * (2 + abs(firstCollision.newVelocity)) * firstCollision.collisionNormal);
            data.setVelocity(firstCollision.newVelocity);
            data.setAcceleration(data.entity->acceleration + data.entity->deltaAcceleration * firstCollision.time);
            if(absSquared(firstCollision.collisionNormal) > eps * eps)
            {
                firstCollision.collisionNormal = normalize(firstCollision.collisionNormal);
                if(dot(data.acceleration, firstCollision.collisionNormal) <= 0)
                {
                    data.entity->acceleration = data.acceleration - firstCollision.collisionNormal * dot(data.acceleration, firstCollision.collisionNormal);
                    data.entity->deltaAcceleration = data.deltaAcceleration - firstCollision.collisionNormal * dot(data.deltaAcceleration, firstCollision.collisionNormal);
                }
            }
            if(supported)
            {
                if(data.entity->velocity.y <= 0)
                {
                    data.entity->velocity.y = 0;
                    data.entity->acceleration.y = max(0.0f, data.entity->acceleration.y);
                }
            }
        }
    }
    if(eData->pclient == nullptr || !isClientValid(*eData->pclient))
    {
        data.clear();
        return;
    }
}
