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
#if 0
#warning finish new_physics
#else
#include "new_physics.h"

namespace Physics
{
namespace
{
inline bool boxesIntersect(PositionF)
}
Contact AABox::getContact(const Object & other_in) const
{
    assert(lastCalcTime == other_in.lastCalcTime);
    if((other_in.properties.contactMask & properties.contactMask) == 0)
        return Contact();

    switch(other_in.type())
    {
    case Type::Empty:
        return Contact();
    case Type::AABox:
    {
        const AABox & other = (const AABox &)other_in;
        if(position.d != other.position.d)
            return Contact();
    }
    }
    assert(false);
    return Contact();
}

Collision AABox::getFirstCollision(const Object & other) const
{

}
}
#endif
