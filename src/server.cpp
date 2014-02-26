#include "server.h"
#include "world.h"
#include "client.h"
#include "network_protocol.h"
#include <thread>
#include <list>

using namespace std;

namespace
{
inline Client::IdType getUpdateListId()
{
    static Client::IdType updateListId = Client::NullId;
    if(updateListId == Client::NullId)
        updateListId = Client::getNewId();
    return updateListId;
}

inline flag & getClientTerminatedFlag(Client &client)
{
    static Client::IdType terminatedId = Client::NullId;
    shared_ptr<flag> retval;
    if(terminatedId == Client::NullId)
    {
        retval = make_shared<flag>(false);
        terminatedId = client.makeId(retval, Client::DataType::ServerFlag);
        return *retval;
    }
    LockedClient lock(client);
    retval = client.getPtr<flag>(terminatedId, Client::DataType::ServerFlag);
    if(retval == nullptr)
    {
        retval = make_shared<flag>(false);
        client.setPtr(retval, terminatedId, Client::DataType::ServerFlag);
    }
    return *retval;
}

void runServerReaderThread(shared_ptr<StreamRW> connection, shared_ptr<Client> pclient, shared_ptr<World> world)
{
    Reader &reader = connection->reader();
    Client &client = *pclient;
    flag &terminated = getClientTerminatedFlag(client);
    try
    {
        terminated.wait(true);
    }
    catch(exception * e)
    {
        cerr << "Error : " << e->what() << endl;
        delete e;
    }
    terminated = true;
}

void runServerWriterThread(shared_ptr<StreamRW> connection, shared_ptr<Client> pclient, shared_ptr<World> world)
{
    Writer &writer = connection->writer();
    Client &client = *pclient;
    flag &terminated = getClientTerminatedFlag(client);
    const int size = 10;
    cout << "connected\n";

    try
    {
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
        while(!terminated)
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
    terminated = true;
}
}

void runServer(StreamServer & server)
{
    list<thread> threads;
    list<shared_ptr<Client>> clients;
    shared_ptr<World> world = make_shared<World>();
    try
    {
        while(true)
        {
            shared_ptr<StreamRW> stream = server.accept();
            shared_ptr<Client> pclient = make_shared<Client>();
            clients.push_back(pclient);
            threads.push_back(thread(runServerWriterThread, stream, pclient, world));
            threads.push_back(thread(runServerReaderThread, stream, pclient, world));
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
