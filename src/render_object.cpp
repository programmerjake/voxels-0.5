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
#include "render_object.h"
#include <iostream>

using namespace std;

void RenderObject::write(Writer &writer, Client &client)
{
    writer.writeU8((uint8_t)type());
    writeInternal(writer, client);
}

shared_ptr<RenderObjectBlockMesh> RenderObjectBlockMesh::read(Reader &reader, Client &client)
{
    Client::IdType id = Client::readIdNonNull(reader);
    shared_ptr<RenderObjectBlockMesh> retval = client.getPtr<RenderObjectBlockMesh>(id, Client::DataType::RenderObjectBlockMesh);

    if(retval != nullptr)
    {
        DUMP_V(RenderObjectBlockMesh::read, "read old block mesh");
        return retval;
    }

    DUMP_V(RenderObjectBlockMesh::read, "reading new block mesh");
    Mesh center, nx, px, ny, py, nz, pz;
    center = readMesh(reader, client);
    nx = readMesh(reader, client);
    px = readMesh(reader, client);
    ny = readMesh(reader, client);
    py = readMesh(reader, client);
    nz = readMesh(reader, client);
    pz = readMesh(reader, client);
    DUMP_V(RenderObjectBlockMesh::read, "read new block meshes");
    LightProperties lightProperties = LightProperties::read(reader);
    RenderLayer rl = readRenderLayer(reader);
    RenderObjectBlockClass blockClass = readRenderObjectBlockClass(reader);
    uint8_t blockedMask = reader.readU8();
    bool nxBlocked = blockedMask & (1 << (int)BlockFace::NX);
    bool pxBlocked = blockedMask & (1 << (int)BlockFace::PX);
    bool nyBlocked = blockedMask & (1 << (int)BlockFace::NY);
    bool pyBlocked = blockedMask & (1 << (int)BlockFace::PY);
    bool nzBlocked = blockedMask & (1 << (int)BlockFace::NZ);
    bool pzBlocked = blockedMask & (1 << (int)BlockFace::PZ);
    VectorF hitBoxMin, hitBoxMax;
    hitBoxMin.x = reader.readLimitedF32(0, 1);
    hitBoxMin.y = reader.readLimitedF32(0, 1);
    hitBoxMin.z = reader.readLimitedF32(0, 1);
    hitBoxMax.x = reader.readLimitedF32(0, 1);
    hitBoxMax.y = reader.readLimitedF32(0, 1);
    hitBoxMax.z = reader.readLimitedF32(0, 1);
    shared_ptr<PhysicsObjectConstructor> physicsConstructor = PhysicsObjectConstructor::read(reader);
    retval = shared_ptr<RenderObjectBlockMesh>(new RenderObjectBlockMesh(blockClass, hitBoxMin, hitBoxMax, lightProperties, center, nx, px, ny, py, nz, pz, nxBlocked, pxBlocked, nyBlocked, pyBlocked, nzBlocked, pzBlocked, rl, physicsConstructor));
    client.setPtr(retval, id, Client::DataType::RenderObjectBlockMesh);
    DUMP_V(RenderObjectBlockMesh::read, "read new block");
    return retval;
}

shared_ptr<RenderObjectBlock> RenderObjectBlock::read(Reader &reader, Client &client)
{
    shared_ptr<RenderObjectBlockMesh> block = RenderObjectBlockMesh::read(reader, client);
    PositionI pos;
    pos.x = reader.readS32();
    pos.y = reader.readS32();
    pos.z = reader.readS32();
    pos.d = reader.readDimension();
    Lighting lighting = Lighting::read(reader);
    //cout << "Read Block : " << pos.x << ", " << pos.y << ", " << pos.z << ", " << (int)pos.d << endl;
    auto retval = shared_ptr<RenderObjectBlock>(new RenderObjectBlock(block, pos, lighting));
    retval->addToClient(client);
    return retval;
}

void RenderObjectBlockMesh::write(Writer &writer, Client &client)
{
    Client::IdType id = client.getId(shared_from_this(), Client::DataType::RenderObjectBlockMesh);

    if(id != Client::NullId)
    {
        Client::writeId(writer, id);
        return;
    }

    id = client.makeId(shared_from_this(), Client::DataType::RenderObjectBlockMesh);
    Client::writeId(writer, id);
    writeMesh(center, writer, client);
    writeMesh(nx, writer, client);
    writeMesh(px, writer, client);
    writeMesh(ny, writer, client);
    writeMesh(py, writer, client);
    writeMesh(nz, writer, client);
    writeMesh(pz, writer, client);
    lightProperties.write(writer);
    writeRenderLayer(rl, writer);
    writeRenderObjectBlockClass(writer, blockClass);
    uint8_t blockedMask = 0;

    if(nxBlocked)
    {
        blockedMask |= 1 << (int)BlockFace::NX;
    }

    if(pxBlocked)
    {
        blockedMask |= 1 << (int)BlockFace::PX;
    }

    if(nyBlocked)
    {
        blockedMask |= 1 << (int)BlockFace::NY;
    }

    if(pyBlocked)
    {
        blockedMask |= 1 << (int)BlockFace::PY;
    }

    if(nzBlocked)
    {
        blockedMask |= 1 << (int)BlockFace::NZ;
    }

    if(pzBlocked)
    {
        blockedMask |= 1 << (int)BlockFace::PZ;
    }
    writer.writeU8(blockedMask);
    writer.writeF32(hitBoxMin.x);
    writer.writeF32(hitBoxMin.y);
    writer.writeF32(hitBoxMin.z);
    writer.writeF32(hitBoxMax.x);
    writer.writeF32(hitBoxMax.y);
    writer.writeF32(hitBoxMax.z);
    physicsConstructor->write(writer);
}

shared_ptr<RenderObject> RenderObject::read(Reader &reader, Client &client)
{
    Type type = (Type)reader.readLimitedU8(0, (uint8_t)Type::Last - 1);

    switch(type)
    {
    case Type::Block:
        return RenderObjectBlock::read(reader, client);

    case Type::Entity:
        return RenderObjectEntity::read(reader, client);

    default:
        throw InvalidDataValueException("read RenderObject type not implemented");
    }
}

void RenderObjectEntity::move(float deltaTime, shared_ptr<RenderObjectWorld> world)
{
    const int searchDist = 2;
    if(!good())
        return;
    updateAge += deltaTime;
    int count = (iceil(deltaTime * abs(velocity) / 0.5 + 1), 1);
    deltaTime /= count;
    RenderObjectWorld::BlockIterator bi = world->get((PositionI)position - VectorI(searchDist));
    for(int step = 0; step < count; step++)
    {
        age += deltaTime;
        int zeroCount = 0;
        while(deltaTime * deltaTime * absSquared(velocity) > eps * eps)
        {
            PhysicsCollision firstCollision(position + deltaTime * velocity + deltaTime * deltaTime * 0.5f * acceleration + deltaTime * deltaTime * deltaTime * (1 / 6.0f) * deltaAcceleration, velocity + deltaTime * acceleration + deltaTime * deltaTime * 0.5f * deltaAcceleration, VectorF(0), deltaTime);
            shared_ptr<PhysicsObject> pphysicsObject = mesh()->constructPhysicsObject(position, velocity, acceleration, deltaAcceleration);
            PhysicsObject & physicsObject = *pphysicsObject;
            for(int dx = -2; dx <= 2; dx++, bi.movePX())
            {
                RenderObjectWorld::BlockIterator bi2 = bi;
                for(int dy = -2; dy <= 2; dy++, bi2.movePY())
                {
                    RenderObjectWorld::BlockIterator curBI = bi2;
                    for(int dz = -2; dz <= 2; dz++, curBI.movePZ())
                    {
                        shared_ptr<PhysicsObject> otherObject;
                        if(curBI.getMesh() != nullptr)
                            otherObject = curBI.getMesh()->constructPhysicsObject(curBI.getPosition());
                        else
                            otherObject = static_pointer_cast<PhysicsObject>(make_shared<PhysicsBox>((VectorI)curBI.getPosition() + VectorF(0.5), VectorF(0.5), VectorF(0), VectorF(0), VectorF(0), curBI.getPosition().d, PhysicsProperties(PhysicsProperties::INFINITE_MASS, 1, 0)));
                        assert(otherObject);
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
            position = firstCollision.newPosition + eps * (2 + abs(firstCollision.newVelocity)) * firstCollision.collisionNormal;
            velocity = firstCollision.newVelocity;
            acceleration += deltaAcceleration * firstCollision.time;
        }
    }
}
