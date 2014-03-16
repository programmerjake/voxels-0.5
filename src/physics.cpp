#include "physics.h"

PhysicsObject::~PhysicsObject()
{
}

namespace
{
inline bool isBoxCollision(VectorF center1, VectorF extents1, VectorF center2, VectorF extents2, VectorF & normal)
{
    VectorF extents = extents1 + extents2 + VectorF(eps);
    VectorF deltaPos = center1 - center2;
    VectorF aDeltaPos = VectorF(abs(deltaPos.x), abs(deltaPos.y), abs(deltaPos.z));
    if(aDeltaPos.x <= extents.x && aDeltaPos.y <= extents.y && aDeltaPos.z <= extents.z)
    {
        VectorF dist = extents - aDeltaPos;
        if(dist.x < dist.y && dist.x < dist.z)
        {
            if(deltaPos.x > 0)
                normal = VectorF(1, 0, 0);
            else
                normal = VectorF(-1, 0, 0);
        }
        else if(dist.y < dist.z)
        {
            if(deltaPos.y > 0)
                normal = VectorF(0, 1, 0);
            else
                normal = VectorF(0, -1, 0);
        }
        else
        {
            if(deltaPos.z > 0)
                normal = VectorF(0, 0, 1);
            else
                normal = VectorF(0, 0, -1);
        }
        return true;
    }
    return false;
}

inline int getCollisions(float times[3], float position1, float velocity1, float acceleration1, float deltaAcceleration1, float position2, float velocity2, float acceleration2, float deltaAcceleration2)
{
    return solveCubic(position1 - position2, velocity1 - velocity2, 1 / 2.0f * (acceleration1 - acceleration2), 1 / 6.0f * (deltaAcceleration1 - deltaAcceleration2), times);
}

inline int getAllCollisions(float times[18], VectorF center1, VectorF velocity1, VectorF acceleration1, VectorF deltaAcceleration1, VectorF extents1, VectorF center2, VectorF velocity2, VectorF acceleration2, VectorF deltaAcceleration2, VectorF extents2)
{
    int count = 0;
    VectorF min1 = center1 - extents1;
    VectorF max1 = center1 + extents1;
    VectorF min2 = center2 - extents2;
    VectorF max2 = center2 + extents2;
    count += getCollisions(&times[count], min1.x, velocity1.x, acceleration1.x, deltaAcceleration1.x, max2.x, velocity2.x, acceleration2.x, deltaAcceleration2.x);
    count += getCollisions(&times[count], min1.y, velocity1.y, acceleration1.y, deltaAcceleration1.y, max2.y, velocity2.y, acceleration2.y, deltaAcceleration2.y);
    count += getCollisions(&times[count], min1.z, velocity1.z, acceleration1.z, deltaAcceleration1.z, max2.z, velocity2.z, acceleration2.z, deltaAcceleration2.z);
    count += getCollisions(&times[count], max1.x, velocity1.x, acceleration1.x, deltaAcceleration1.x, min2.x, velocity2.x, acceleration2.x, deltaAcceleration2.x);
    count += getCollisions(&times[count], max1.y, velocity1.y, acceleration1.y, deltaAcceleration1.y, min2.y, velocity2.y, acceleration2.y, deltaAcceleration2.y);
    count += getCollisions(&times[count], max1.z, velocity1.z, acceleration1.z, deltaAcceleration1.z, min2.z, velocity2.z, acceleration2.z, deltaAcceleration2.z);
    return count;
}

inline float getFirstActualCollision(VectorF center1, VectorF velocity1, VectorF acceleration1, VectorF deltaAcceleration1, VectorF extents1, VectorF center2, VectorF velocity2, VectorF acceleration2, VectorF deltaAcceleration2, VectorF extents2, VectorF & collisionNormal)
{
    float times[18];
    int count = getAllCollisions(times, center1, velocity1, acceleration1, deltaAcceleration1, extents1, center2, velocity2, acceleration2, deltaAcceleration2, extents2);
    float minimum = -1;
    for(int i = 0; i < count; i++)
    {
        if(times[i] < 0)
            continue;
        float t = times[i];
        VectorF pos1 = center1 + t * velocity1 + t * t * (1 / 2.0f) * acceleration1 + t * t * t * (1 / 6.0f) * deltaAcceleration1;
        VectorF pos2 = center2 + t * velocity2 + t * t * (1 / 2.0f) * acceleration2 + t * t * t * (1 / 6.0f) * deltaAcceleration2;
        VectorF curNormal;
        if(isBoxCollision(pos1, extents1, pos2, extents2, curNormal))
        {
            if(minimum == -1 || t < minimum)
            {
                minimum = t;
                collisionNormal = curNormal;
            }
        }
    }
    return minimum;
}
}

PhysicsCollision PhysicsBox::collide(shared_ptr<const PhysicsObject> pother, float maxTime) const
{
    if(pother == nullptr)
        return PhysicsCollision();
    if(pother->dimension() != dimension())
        return PhysicsCollision();
    switch(pother->type())
    {
    case Type::None:
        return PhysicsCollision();
    case Type::Box:
    {
        const PhysicsBox & other = *dynamic_cast<const PhysicsBox *>(pother.get());
        VectorF min = center - extents;
        VectorF max = center + extents;
        VectorF otherMin = other.center - other.extents;
        VectorF otherMax = other.center + other.extents;
        if(min.x < otherMax.x && min.y < otherMax.y && min.z < otherMax.z && max.x > otherMin.x && max.y > otherMin.y && max.z > otherMin.z) // intersects now
        {
            VectorF newVelocity = (velocity * properties().mass + other.velocity * other.properties().mass) / (properties().mass + other.properties().mass);
            VectorF deltaCenter = center - other.center;
            VectorF deltaFixedPosition;
            if(deltaCenter.x > 0)
            {
                deltaFixedPosition.x = otherMax.x - min.x;
            }
            else
            {
                deltaFixedPosition.x = otherMin.x - max.x;
            }
            if(deltaCenter.y > 0)
            {
                deltaFixedPosition.y = otherMax.y - min.y;
            }
            else
            {
                deltaFixedPosition.y = otherMin.y - max.y;
            }
            if(deltaCenter.z > 0)
            {
                deltaFixedPosition.z = otherMax.z - min.z;
            }
            else
            {
                deltaFixedPosition.z = otherMin.z - max.z;
            }
            VectorF fixedPosition = center;
            if(abs(deltaCenter.x) > abs(deltaCenter.y))
            {
                if(abs(deltaCenter.x) > abs(deltaCenter.z))
                {
                    fixedPosition.x += deltaFixedPosition.x;
                }
                else
                {
                    fixedPosition.z += deltaFixedPosition.z;
                }
            }
            else if(abs(deltaCenter.y) > abs(deltaCenter.z))
            {
                fixedPosition.y += deltaFixedPosition.y;
            }
            else
            {
                fixedPosition.z += deltaFixedPosition.z;
            }
            VectorF newPosition = (center * properties().mass + fixedPosition * other.properties().mass) / (properties().mass + other.properties().mass);
            return PhysicsCollision(PositionF(newPosition, dimension()), newVelocity, normalize(newPosition - center), 0);
        }
#if 0
        Ray ray(velocity - other.velocity, PositionF(center, dimension()));
        if(!ray.good())
            return PhysicsCollision();
        BoxRayCollision brc = rayHitBox(other.center - extents - other.extents, other.center + extents + other.extents, ray);
        if(!brc.good())
            return PhysicsCollision();
        float collisionTime = brc.t;
        VectorF collisionNormal = VectorF(dx(brc.enterFace), dy(brc.enterFace), dz(brc.enterFace));
        VectorF cVelocity = velocity, cPosition = velocity * collisionTime + center;
        VectorF oCVelocity = other.velocity, oCPosition = other.velocity * collisionTime + other.center;
#else
        VectorF collisionNormal;
        float collisionTime = getFirstActualCollision(center, velocity, acceleration, deltaAcceleration, extents, other.center, other.velocity, other.acceleration, other.deltaAcceleration, other.extents, collisionNormal);
        if(collisionTime < 0)
            return PhysicsCollision();
        VectorF cVelocity = velocity + acceleration * collisionTime + collisionTime * collisionTime * 0.5f * deltaAcceleration;
        VectorF cPosition = center + velocity * collisionTime + collisionTime * collisionTime * 0.5f * acceleration + collisionTime * collisionTime * collisionTime * (1 / 6.0f) * deltaAcceleration;
        VectorF oCVelocity = other.velocity + other.acceleration * collisionTime + collisionTime * collisionTime * 0.5f * other.deltaAcceleration;
        //VectorF oCPosition = other.center + other.velocity * collisionTime + collisionTime * collisionTime * 0.5f * other.acceleration + collisionTime * collisionTime * collisionTime * (1 / 6.0f) * other.deltaAcceleration;
#endif
        if(collisionTime > maxTime)
            return PhysicsCollision();
        VectorF nonReflectedVelocity = (cVelocity * properties().mass + oCVelocity * other.properties().mass) / (properties().mass + other.properties().mass);
        VectorF reflectedVelocity = (cVelocity * (properties().mass - other.properties().mass) + (other.properties().mass + other.properties().mass) * oCVelocity) / (properties().mass + other.properties().mass);
        VectorF partiallyReflectedVelocity = nonReflectedVelocity + properties().bounciness * other.properties().bounciness * (reflectedVelocity - nonReflectedVelocity);
        float reflectedDot = dot(partiallyReflectedVelocity, collisionNormal);
        VectorF frictionlessVelocity = cVelocity;
        VectorF stuckVelocity = nonReflectedVelocity;
        VectorF newVelocity = frictionlessVelocity + properties().friction * other.properties().friction * (stuckVelocity - frictionlessVelocity);
        newVelocity -= collisionNormal * dot(newVelocity, collisionNormal);
        newVelocity += collisionNormal * reflectedDot;
        VectorF newPosition = cPosition;
        return PhysicsCollision(PositionF(newPosition, dimension()), newVelocity, collisionNormal, collisionTime);
    }
    }
    assert(false);
    return PhysicsCollision();
}

