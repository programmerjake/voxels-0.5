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
    RequestState,
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
