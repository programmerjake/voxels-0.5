#include "entity_block.h"
#include "block.h"

void initEntityBlock()
{
    EntityDescriptor::initEntity(shared_ptr<EntityDescriptor>(new EntityBlock()));
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
                theScript = Script::parse(L"io.transform = make_translate(<-0.5, -0.5, -0.5>) ~ make_scale(0.125) ~ make_rotatey(io.age / 5 * 2 * pi) ~ make_translate(io.position);");
            mesh = make_shared<RenderObjectEntityMesh>(VectorF(0), VectorF(0));
            mesh->addPart(eData->block->makeBlockEntityMesh(), theScript);
        }
        data.entity = make_shared<RenderObjectEntity>(mesh, data.position, data.velocity, 0);
    }
    return data.entity;
}

void EntityBlock::onMove(EntityData & data, shared_ptr<World> world, float deltaTime) const
{
    getEntity(data, world);
    int count = iceil(deltaTime * abs(data.velocity) / 0.5);
    deltaTime /= count;
    BlockIterator bi = world->get((PositionI)data.position);
    PhysicsBox & physicsObject = *make_shared<PhysicsBox>((VectorF)data.position, VectorF(0.0625), data.position.d, PhysicsProperties(0.1, 0.5, 0.1));
    for(int step = 0; step < count; step++)
    {
        data.setVelocity(data.velocity + deltaTime * gravityVector);
        while(deltaTime > eps)
        {
            PhysicsCollision firstCollision(data.position + deltaTime * data.velocity, data.velocity, deltaTime);
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
                            otherObject = static_pointer_cast<PhysicsObject>(make_shared<PhysicsBox>((VectorI)curBI.position() + VectorF(0.5), VectorF(0.5), curBI.position().d, PhysicsProperties(PhysicsProperties::INFINITE_MASS, 1, 0)));
                        assert(otherObject);
                        PhysicsCollision collision = physicsObject.collide(otherObject, deltaTime);
                        if(collision.valid && collision.time < firstCollision.time)
                            firstCollision = collision;
                    }
                }
            }
            deltaTime -= firstCollision.time;
            data.setPosition(firstCollision.newPosition);
            data.setVelocity(firstCollision.newVelocity);
        }
    }
}
