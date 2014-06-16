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
#include "util.h"
#include <algorithm>

using namespace std;

namespace Physics
{
namespace
{
inline bool boxCollides(VectorF & position, VectorF other_position, VectorF size, VectorF other_size, VectorF & normal)
{
    VectorF minDisplacement = 0.5 * (size + other_size);
    VectorF displacement = position - other_position;
    VectorF absDisplacement = VectorF(abs(displacement.x), abs(displacement.y), abs(displacement.z));
    if(absDisplacement.x - ContactEPS >= minDisplacement.x)
        return false;
    if(absDisplacement.y - ContactEPS >= minDisplacement.y)
        return false;
    if(absDisplacement.z - ContactEPS >= minDisplacement.z)
        return false;
    VectorF surfaceDisplacement = absDisplacement - minDisplacement;
    if(surfaceDisplacement.x > surfaceDisplacement.y && surfaceDisplacement.x > surfaceDisplacement.z)
    {
        if(displacement.x < 0)
        {
            position.x = other_position.x - minDisplacement.x - ContactEPS * 0.5;
            normal = VectorF(-1, 0, 0);
            return true;
        }
        else
        {
            position.x = other_position.x + minDisplacement.x + ContactEPS * 0.5;
            normal = VectorF(1, 0, 0);
            return true;
        }
    }
    else if(surfaceDisplacement.y > surfaceDisplacement.z)
    {
        if(displacement.y < 0)
        {
            position.y = other_position.y - minDisplacement.y - ContactEPS * 0.5;
            normal = VectorF(0, -1, 0);
            return true;
        }
        else
        {
            position.y = other_position.y + minDisplacement.y + ContactEPS * 0.5;
            normal = VectorF(0, 1, 0);
            return true;
        }
    }
    else
    {
        if(displacement.z < 0)
        {
            position.z = other_position.z - minDisplacement.z - ContactEPS * 0.5;
            normal = VectorF(0, 0, -1);
            return true;
        }
        else
        {
            position.z = other_position.z + minDisplacement.z + ContactEPS * 0.5;
            normal = VectorF(0, 0, 1);
            return true;
        }
    }
}
}
bool AABox::processContact(const Object & other_in)
{
    assert(lastCalcTime == other_in.lastCalcTime);
    if((properties.contactMask1 & other_in.properties.contactMask2) == 0)
        return false;

    switch(other_in.type())
    {
    case Type::Empty:
        return false;
    case Type::AABox:
    {
        const AABox & other = dynamic_cast<const AABox &>(other_in);
        if(position.d != other.position.d)
            return false;
        VectorF p = position;
        VectorF normal;
        if(!boxCollides(p, other.position, size, other.size, normal))
            return false;
        bool retval = (p != position);
        position = p + 0.5 * (position - p);
        float velocityDot = dot(velocity - other.velocity, normal);
        if(velocityDot >= eps)
            return retval;
        velocity -= velocityDot * normal;
        float accelerationDot = dot(acceleration - other.acceleration, normal);
        if(accelerationDot >= eps)
            return true;
        acceleration -= accelerationDot * normal;
        return true;
    }
    }
    assert(false);
    return false;
}

Collision AABox::getFirstCollision(const Object & other_in) const
{
    if((other_in.properties.contactMask & properties.contactMask) == 0)
        return Collision();

    switch(other_in.type())
    {
    case Type::Empty:
        return Collision();
    case Type::AABox:
    {
        const AABox & other = dynamic_cast<const AABox &>(other_in);
        if(position.d != other.position.d)
            return Collision();
        PositionF testOtherPosition = other.position;
        VectorF testOtherVelocity = other.velocity;
        double timeOffset = lastCalcTime;
        PositionF testPosition = position;
        VectorF testVelocity = velocity;
        if(lastCalcTime < other.lastCalcTime)
        {
            timeOffset = other.lastCalcTime;
            float deltaTime = other.lastCalcTime - lastCalcTime;
            testPosition += testVelocity * deltaTime + 0.5f * deltaTime * deltaTime * acceleration;
            testVelocity += acceleration * deltaTime;
        }
        else if(lastCalcTime > other.lastCalcTime)
        {
            float deltaTime = lastCalcTime - other.lastCalcTime;
            testOtherPosition += testOtherVelocity * deltaTime + 0.5f * deltaTime * deltaTime * other.acceleration;
            testOtherVelocity += other.acceleration * deltaTime;
        }
        VectorF minDisplacement = 0.5 * (size + other.size);
        const int rootsPerEquation = 2, numberOfAxes = 3, facesPerAxis = 2;
        float collisionTimes[rootsPerEquation * numberOfAxes * facesPerAxis];
        int possibleCollisionCount = 0;
        possibleCollisionCount += solveQuadratic(testPosition.x - testOtherPosition.x + minDisplacement.x, testVelocity.x - testOtherVelocity.x, acceleration.x - other.acceleration.x, &collisionTimes[possibleCollisionCount]);
        possibleCollisionCount += solveQuadratic(testPosition.x - testOtherPosition.x - minDisplacement.x, testVelocity.x - testOtherVelocity.x, acceleration.x - other.acceleration.x, &collisionTimes[possibleCollisionCount]);
        possibleCollisionCount += solveQuadratic(testPosition.y - testOtherPosition.y + minDisplacement.y, testVelocity.y - testOtherVelocity.y, acceleration.y - other.acceleration.y, &collisionTimes[possibleCollisionCount]);
        possibleCollisionCount += solveQuadratic(testPosition.y - testOtherPosition.y - minDisplacement.y, testVelocity.y - testOtherVelocity.y, acceleration.y - other.acceleration.y, &collisionTimes[possibleCollisionCount]);
        possibleCollisionCount += solveQuadratic(testPosition.z - testOtherPosition.z + minDisplacement.z, testVelocity.z - testOtherVelocity.z, acceleration.z - other.acceleration.z, &collisionTimes[possibleCollisionCount]);
        possibleCollisionCount += solveQuadratic(testPosition.z - testOtherPosition.z - minDisplacement.z, testVelocity.z - testOtherVelocity.z, acceleration.z - other.acceleration.z, &collisionTimes[possibleCollisionCount]);
        sort(begin(collisionTimes), end(collisionTimes));
        VectorF normal;
        for(float t : collisionTimes)
        {
            if(t < eps)
                continue;
            VectorF p = testPosition + testVelocity * t + t * t * 0.5 * acceleration, v = testVelocity + t * acceleration, normal;
            if(!boxCollides(p, testOtherPosition + testOtherVelocity * t + t * t * 0.5 * other.acceleration, size, other.size, normal))
                continue;
            return Collision(p, normal, testVelocity + acceleration * t, t + timeOffset);
        }
        return Collision();
    }
    }
    assert(false);
    return Collision();
}

void AABox::handleFirstCollision(Collision c, const Object &)
{
    if(!c.good())
        return;
    lastCalcTime = c.time;
    position = c.position;
    velocity = c.velocity;
}

void World::move(double runToTime)
{
    while(currentTime < runToTime)
    {
#warning finish Physics::World::move()
        assert(false);
    }
}
}
#endif
