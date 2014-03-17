#ifndef PHYSICS_H_INCLUDED
#define PHYSICS_H_INCLUDED

#include "position.h"
#include "util.h"
#include "ray_casting.h"
#include <memory>

using namespace std;

struct PhysicsCollision final
{
    PositionF newPosition;
    VectorF newVelocity;
    VectorF collisionNormal;
    bool valid;
    float time;
    PhysicsCollision()
        : valid(false), time(-1)
    {
    }
    PhysicsCollision(PositionF newPosition, VectorF newVelocity, VectorF collisionNormal, float time)
        : newPosition(newPosition), newVelocity(newVelocity), collisionNormal(collisionNormal), valid(true), time(time)
    {
    }
};

struct PhysicsProperties final
{
    float mass, friction, bounciness;
    static constexpr float INFINITE_MASS = 1e20;
    PhysicsProperties(float mass, float friction, float bounciness)
        : mass(mass), friction(friction), bounciness(bounciness)
    {
    }
};

class PhysicsObject : public enable_shared_from_this<PhysicsObject>
{
    const PhysicsObject & operator =(const PhysicsObject & rt) = delete;
    PhysicsObject(PhysicsObject && rt) = delete;
    PhysicsObject(const PhysicsObject & rt) = default;
private:
    Dimension d;
    PhysicsProperties props;
public:
    Dimension dimension() const
    {
        return d;
    }
    const PhysicsProperties & properties() const
    {
        return props;
    }

    virtual ~PhysicsObject();
    PhysicsObject(Dimension dimension, PhysicsProperties properties)
        : d(dimension), props(properties)
    {
    }
    virtual PhysicsCollision collide(shared_ptr<const PhysicsObject> pother, float maxTime) const = 0;
    enum class Type
    {
        None,
        Box
    };
    virtual Type type() const = 0;
};

class PhysicsEmpty final : public PhysicsObject
{
public:
    PhysicsEmpty()
        : PhysicsObject(Dimension::Overworld, PhysicsProperties(1, 0, 0))
    {
    }
    virtual Type type() const override
    {
        return Type::None;
    }
    virtual PhysicsCollision collide(shared_ptr<const PhysicsObject>, float) const
    {
        return PhysicsCollision();
    }
};

class PhysicsBox final : public PhysicsObject
{
public:
    VectorF center, extents;
    VectorF velocity, acceleration, deltaAcceleration;
    PhysicsBox(VectorF center, VectorF extents/** size is 2 * extents */, VectorF velocity, VectorF acceleration, VectorF deltaAcceleration, Dimension dimension, PhysicsProperties properties)
        : PhysicsObject(dimension, properties), center(center), extents(extents), velocity(velocity), acceleration(acceleration), deltaAcceleration(deltaAcceleration)
    {
        assert(extents.x > eps && extents.y > eps && extents.z > eps);
    }
    void reInit(VectorF center, VectorF extents/** size is 2 * extents */, VectorF velocity, VectorF acceleration, VectorF deltaAcceleration)
    {
        this->center = center;
        this->extents = extents;
        this->velocity = velocity;
        this->acceleration = acceleration;
        this->deltaAcceleration = deltaAcceleration;
    }
    virtual PhysicsCollision collide(shared_ptr<const PhysicsObject> pother, float maxTime) const override;
    virtual Type type() const override
    {
        return Type::Box;
    }
};

#endif // PHYSICS_H_INCLUDED
