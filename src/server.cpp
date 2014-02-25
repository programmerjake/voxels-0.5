#include "server.h"
#include "world.h"
#include "client.h"
#include "network_protocol.h"
#include "texture_atlas.h"
#include "generate.h"
#include <thread>
#include <list>

using namespace std;

namespace
{
void runServerThread(shared_ptr<StreamRW> connection)
{
    const int size = 10;
    cout << "connected\n";

    try
    {
        Reader &reader = connection->reader();
        Writer &writer = connection->writer();
        Client client;
        shared_ptr<RenderObjectBlockMesh> air, oakWood, birchWood, spruceWood, jungleWood, glass, water;
        air = make_shared<RenderObjectBlockMesh>(0, LightProperties(LightPropertiesType::Transparent, 15), Mesh(new Mesh_t), Mesh(new Mesh_t), Mesh(new Mesh_t), Mesh(new Mesh_t), Mesh(new Mesh_t), Mesh(new Mesh_t), Mesh(new Mesh_t), false, false, false, false, false, false, RenderLayer::Opaque);
        oakWood = make_shared<RenderObjectBlockMesh>(1, LightProperties(LightPropertiesType::Opaque, 0), Mesh(new Mesh_t),
                Generate::unitBox(TextureAtlas::OakWood.td(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureAtlas::OakWood.td(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureAtlas::WoodEnd.td(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureAtlas::WoodEnd.td(), TextureDescriptor(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureAtlas::OakWood.td(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureAtlas::OakWood.td()),
                true, true, true, true, true, true, RenderLayer::Opaque
                                                 );
        birchWood = make_shared<RenderObjectBlockMesh>(1, LightProperties(LightPropertiesType::Opaque, 0), Mesh(new Mesh_t),
                Generate::unitBox(TextureAtlas::BirchWood.td(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureAtlas::BirchWood.td(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureAtlas::WoodEnd.td(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureAtlas::WoodEnd.td(), TextureDescriptor(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureAtlas::BirchWood.td(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureAtlas::BirchWood.td()),
                true, true, true, true, true, true, RenderLayer::Opaque
                                                 );
        spruceWood = make_shared<RenderObjectBlockMesh>(1, LightProperties(LightPropertiesType::Opaque, 0), Mesh(new Mesh_t),
                Generate::unitBox(TextureAtlas::SpruceWood.td(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureAtlas::SpruceWood.td(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureAtlas::WoodEnd.td(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureAtlas::WoodEnd.td(), TextureDescriptor(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureAtlas::SpruceWood.td(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureAtlas::SpruceWood.td()),
                true, true, true, true, true, true, RenderLayer::Opaque
                                                 );
        jungleWood = make_shared<RenderObjectBlockMesh>(1, LightProperties(LightPropertiesType::Opaque, 0), Mesh(new Mesh_t),
                Generate::unitBox(TextureAtlas::JungleWood.td(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureAtlas::JungleWood.td(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureAtlas::WoodEnd.td(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureAtlas::WoodEnd.td(), TextureDescriptor(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureAtlas::JungleWood.td(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureAtlas::JungleWood.td()),
                true, true, true, true, true, true, RenderLayer::Opaque
                                                 );
        glass = make_shared<RenderObjectBlockMesh>(2, LightProperties(LightPropertiesType::Transparent, 0), Mesh(new Mesh_t),
                Generate::unitBox(TextureAtlas::Glass.td(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureAtlas::Glass.td(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureAtlas::Glass.td(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureAtlas::Glass.td(), TextureDescriptor(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureAtlas::Glass.td(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureAtlas::Glass.td()),
                false, false, false, false, false, false, RenderLayer::Opaque
                                                 );
        water = make_shared<RenderObjectBlockMesh>(3, LightProperties(LightPropertiesType::Water, 0), Mesh(new Mesh_t),
                Generate::unitBox(TextureAtlas::WaterSide0.td(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureAtlas::WaterSide1.td(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureAtlas::WaterSide2.td(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureAtlas::WaterSide3.td(), TextureDescriptor(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureAtlas::WaterSide4.td(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureAtlas::WaterSide5.td()),
                false, false, false, false, false, false, RenderLayer::Translucent
                                                 );
        vector<shared_ptr<RenderObjectBlock>> objects;
        for(int dx = -size; dx <= size; dx++)
        {
            for(int dy = -size; dy <= size; dy++)
            {
                for(int dz = -size; dz <= size; dz++)
                {
                    shared_ptr<RenderObjectBlock> block = make_shared<RenderObjectBlock>((dx * dx + dy * dy + dz * dz >= size * size) ? oakWood : air, PositionI(dx, dy, dz, Dimension::Overworld));
                    block->addToClient(client);
                    objects.push_back(block);
                }
            }
        }
        for(shared_ptr<RenderObject> object : objects)
        {
            NetworkProtocol::writeNetworkEvent(writer, NetworkProtocol::NetworkEvent::UpdateRenderObjects);
            writer.flush();
            writer.writeU64(1);
            object->write(writer, client);
        }
        shared_ptr<RenderObjectBlockMesh> blocks[] =
        {
            air,
            oakWood,
            birchWood,
            spruceWood,
            jungleWood,
            glass,
            water
        };
        writer.flush();
        int count = 0;
        while(true)
        {
            int dx = rand() % (size * 2 + 1 - 2) - size + 1;
            int dy = rand() % (size * 2 + 1 - 2) - size + 1;
            int dz = rand() % (size * 2 + 1 - 2) - size + 1;
            if(dx * dx + dy * dy + dz * dz < size * size)
                continue;
            if(count++ > 20)
            {
                count = 0;
                this_thread::sleep_for(chrono::milliseconds(10));
            }
            shared_ptr<RenderObjectBlock> block = make_shared<RenderObjectBlock>(blocks[rand() % (sizeof(blocks) / sizeof(blocks[0]))], PositionI(dx, dy, dz, Dimension::Overworld));
            block->addToClient(client);
            NetworkProtocol::writeNetworkEvent(writer, NetworkProtocol::NetworkEvent::UpdateRenderObjects);
            writer.writeU64(1);
            block->write(writer, client);
            writer.flush();
        }
    }
    catch(exception * e)
    {
        cerr << "Error : " << e->what() << endl;
        delete e;
    }
}
}

void runServer(StreamServer & server)
{
    list<thread> threads;
    try
    {
        while(true)
        {
            threads.push_back(thread(runServerThread, server.accept()));
        }
    }
    catch(NoStreamsLeftException * e)
    {
        delete e;
    }
    for(thread & t : threads)
    {
        t.join();
    }
}
