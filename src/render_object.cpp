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
    retval = shared_ptr<RenderObjectBlockMesh>(new RenderObjectBlockMesh(blockClass, lightProperties, center, nx, px, ny, py, nz, pz, nxBlocked, pxBlocked, nyBlocked, pyBlocked, nzBlocked, pzBlocked, rl));
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
    //cout << "Read Block : " << pos.x << ", " << pos.y << ", " << pos.z << ", " << (int)pos.d << endl;
    auto retval = shared_ptr<RenderObjectBlock>(new RenderObjectBlock(block, pos));
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
}

shared_ptr<RenderObject> RenderObject::read(Reader &reader, Client &client)
{
    Type type = (Type)reader.readLimitedU8(0, (uint8_t)Type::Last);

    switch(type)
    {
    case Type::Block:
        return RenderObjectBlock::read(reader, client);

    default:
        throw new InvalidDataValueException("read RenderObject type not implemented");
    }
}

