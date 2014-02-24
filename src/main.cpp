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
#include <iostream>
#include <sstream>
#include "platform.h"
#include "generate.h"
#include "texture_atlas.h"
#include "text.h"
#include "game_version.h"
#include "event.h"
#include "util.h"
#include "render_object.h"
#include "client.h"
#include "network_protocol.h"
#include <thread>
#include <chrono>

using namespace std;

const int size = 4;

void sourceThreadFn(Reader *preader, Writer *pwriter)
{
    Reader &reader = *preader;
    Writer &writer = *pwriter;
    Client client;
    shared_ptr<RenderObjectBlockMesh> air, wood;
    air = make_shared<RenderObjectBlockMesh>(LightProperties(LightPropertiesType::Transparent, 15), Mesh(new Mesh_t), Mesh(new Mesh_t), Mesh(new Mesh_t), Mesh(new Mesh_t), Mesh(new Mesh_t), Mesh(new Mesh_t), Mesh(new Mesh_t), false, false, false, false, false, false, RenderLayer::Opaque);
    wood = make_shared<RenderObjectBlockMesh>(LightProperties(LightPropertiesType::Opaque, 0), Mesh(new Mesh_t),
            Generate::unitBox(TextureAtlas::OakWood.td(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor()),
            Generate::unitBox(TextureDescriptor(), TextureAtlas::OakWood.td(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor()),
            Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureAtlas::WoodEnd.td(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor()),
            Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureAtlas::WoodEnd.td(), TextureDescriptor(), TextureDescriptor()),
            Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureAtlas::OakWood.td(), TextureDescriptor()),
            Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureAtlas::OakWood.td()),
            true, true, true, true, true, true, RenderLayer::Opaque
                                             );
    vector<shared_ptr<RenderObjectBlock>> objects;
    for(int dx = -size; dx <= size; dx++)
    {
        for(int dy = -size; dy <= size; dy++)
        {
            for(int dz = -size; dz <= size; dz++)
            {
                shared_ptr<RenderObjectBlock> block = make_shared<RenderObjectBlock>((dx * dx + dy * dy + dz * dz < size * size) ? wood : air, PositionI(dx, dy, dz, Dimension::Overworld));
                block->addToClient(client);
                objects.push_back(block);
            }
        }
    }
    NetworkProtocol::writeNetworkEvent(writer, NetworkProtocol::NetworkEvent::UpdateRenderObjects);
    writer.writeU64(objects.size());
    for(shared_ptr<RenderObject> object : objects)
    {
        object->write(writer, client);
    }
    writer.flush();
    while(true)
    {
        this_thread::sleep_for(chrono::milliseconds(10));
        int dx = rand() % (size * 2 + 1 - 2) - size + 1;
        int dy = rand() % (size * 2 + 1 - 2) - size + 1;
        int dz = rand() % (size * 2 + 1 - 2) - size + 1;
        shared_ptr<RenderObjectBlock> block = make_shared<RenderObjectBlock>((rand() % 2) ? wood : air, PositionI(dx, dy, dz, Dimension::Overworld));
        block->addToClient(client);
        NetworkProtocol::writeNetworkEvent(writer, NetworkProtocol::NetworkEvent::UpdateRenderObjects);
        writer.writeU64(1);
        block->write(writer, client);
        writer.flush();
    }
}

int main()
{
    StreamPipe pipe1, pipe2;
    //DumpingReader dumpRead(pipe2.reader());
    thread sourceThread(sourceThreadFn, &pipe1.reader(), &pipe2.writer());
    clientProcess(pipe2.reader(), pipe1.writer());
    return 0;
}
