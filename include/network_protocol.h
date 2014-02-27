#ifndef NETWORK_PROTOCOL_H_INCLUDED
#define NETWORK_PROTOCOL_H_INCLUDED

#include <cstdint>
#include "stream.h"

namespace NetworkProtocol
{

enum class NetworkEvent : uint_fast8_t
{
    UpdateRenderObjects,
    UpdatePositionAndVelocity,
    RequestChunk,
    Last
};

typedef NetworkEvent NetworkEvent;

inline NetworkEvent readNetworkEvent(Reader & reader)
{
    return (NetworkEvent)reader.readLimitedU8(0, (uint8_t)NetworkEvent::Last);
}

inline void writeNetworkEvent(Writer & writer, NetworkEvent event)
{
    writer.writeU8((uint8_t)event);
}

}

#endif // NETWORK_PROTOCOL_H_INCLUDED
