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
#include "entity_block.h"
#include "block.h"

#error finish changing to new physics engine

void initEntityBlock()
{
    EntityDescriptor::initEntity(shared_ptr<EntityDescriptor>(new EntityBlock()));
}

namespace
{
initializer init1([](){initEntityBlock();});
}

shared_ptr<RenderObjectEntity> EntityBlock::getEntity(EntityData & data, shared_ptr<World>) const
{
    assert(data.extraData);
    auto eData = dynamic_pointer_cast<ExtraData>(data.extraData);
    assert(eData);
    if(data.entity == nullptr)
    {
        static shared_ptr<unordered_map<BlockDescriptorPtr, shared_ptr<RenderObjectEntityMesh>>> meshs_static = nullptr;
        if(meshs_static == nullptr)
        {
            meshs_static = make_shared<unordered_map<BlockDescriptorPtr, shared_ptr<RenderObjectEntityMesh>>>();
        }
        unordered_map<BlockDescriptorPtr, shared_ptr<RenderObjectEntityMesh>> & meshs = *meshs_static;
        shared_ptr<RenderObjectEntityMesh> & mesh = meshs[eData->block];
        if(mesh == nullptr)
        {
            static shared_ptr<Script> theScript = nullptr;
            if(theScript == nullptr)
                theScript = Script::parse(L"io.transform = make_translate(<-0.5, -0.5, -0.5>) ~ make_scale(0.25) ~ make_rotatey(io.age / 5 * 2 * pi) ~ make_translate(io.position)");
            mesh = make_shared<RenderObjectEntityMesh>(VectorF(0), VectorF(0), getPhysicsObjectConstructor(data));
            mesh->addPart(eData->block->makeBlockEntityMesh(), theScript);
        }
        data.entity = make_shared<RenderObjectEntity>(mesh, data.position, data.velocity, data.acceleration, data.deltaAcceleration, 0);
    }
    return data.entity;
}

void EntityBlock::onMove(EntityData & data, shared_ptr<World> world, float deltaTimeIn) const
{
    getEntity(data, world);
    data.deltaAcceleration = VectorF(0);
    data.acceleration = gravityVector;
    int count = iceil(deltaTimeIn * abs(data.velocity) / 0.5 + 1);
    BlockIterator bi = world->get((PositionI)data.position);
    data.entity->acceleration = data.acceleration;
    data.entity->deltaAcceleration = data.deltaAcceleration;
    auto pphysicsObject = make_shared<PhysicsBox>((VectorF)data.position, physicsExtents(), data.velocity, data.entity->acceleration, data.entity->deltaAcceleration, data.position.d, physicsProperties());
    PhysicsBox & physicsObject = *pphysicsObject;
    for(int step = 0; step < count; step++)
    {
        float deltaTime = deltaTimeIn / count;
        data.entity->age += deltaTime;
        int zeroCount = 0;
        while(deltaTime * deltaTime * absSquared(data.velocity) > eps * eps)
        {
            bool supported = false;
            PhysicsCollision firstCollision(data.position + deltaTime * data.velocity + deltaTime * deltaTime * 0.5f * data.entity->acceleration + deltaTime * deltaTime * deltaTime * (1 / 6.0f) * data.entity->deltaAcceleration, data.velocity + deltaTime * data.entity->acceleration + deltaTime * deltaTime * 0.5f * data.entity->deltaAcceleration, VectorF(0), deltaTime);
            physicsObject.reInit((VectorF)data.position, physicsExtents(), data.velocity, data.entity->acceleration, data.entity->deltaAcceleration);
            for(int dx = -1; dx <= 1; dx++)
            {
                for(int dy = -1; dy <= 1; dy++)
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
                                    newY = max.y + physicsObject.extents.y;
                                    filled = true;
                                }
                                VectorF temp;
                                if(isBoxCollision(pbox->center, pbox->extents, physicsObject.center - VectorF(0, eps * 10, 0), physicsObject.extents, temp) && !isBoxCollision(pbox->center, pbox->extents, physicsObject.center, physicsObject.extents, temp))
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
        }
    }
    if(data.entity->age > 15)
    {
        data.clear();
    }
}
