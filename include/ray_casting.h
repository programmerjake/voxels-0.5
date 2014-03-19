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
#ifndef RAY_CASTING_H_INCLUDED
#define RAY_CASTING_H_INCLUDED

#include "position.h"
#include "block_face.h"
#include "util.h"

struct Ray final
{
    VectorF direction;
    PositionF position;
    Ray()
        : direction(0), position(VectorF(0), Dimension::Overworld)
    {
    }
    Ray(VectorF direction, PositionF position)
        : direction(direction), position(position)
    {
    }
    bool good() const
    {
        return abs(direction.x) >= eps || abs(direction.y) >= eps || abs(direction.z) >= eps;
    }
};

struct BoxRayCollision final
{
    float t;
    BlockFace enterFace;
    BlockFace exitFace;
    bool good() const
    {
        return t >= eps;
    }
    BoxRayCollision()
        : t(-1), enterFace(BlockFace::NX), exitFace(BlockFace::PX)
    {
    }
    BoxRayCollision(float t, BlockFace enterFace, BlockFace exitFace)
        : t(t), enterFace(enterFace), exitFace(exitFace)
    {
    }
};

inline BoxRayCollision rayHitBox(VectorF min, VectorF max, Ray ray)
{
    assert(min.x < max.x && min.y < max.y && min.z < max.z && ray.good());
    if(abs(ray.direction.x) < eps)
        ray.direction.x = eps;
    if(abs(ray.direction.y) < eps)
        ray.direction.y = eps;
    if(abs(ray.direction.z) < eps)
        ray.direction.z = eps;
    VectorF collidePlane = min;
    if(ray.direction.x < 0)
        collidePlane.x = max.x;
    if(ray.direction.y < 0)
        collidePlane.y = max.y;
    if(ray.direction.z < 0)
        collidePlane.z = max.z;
    VectorF t = (collidePlane - (VectorF)ray.position) / ray.direction;
    if(t.x >= eps)
    {
        VectorF p = ray.direction * t.x + (VectorF)ray.position;
        if(p.y < min.y || p.y > max.y || p.z < min.z || p.z > max.z)
            t.x = -1;
    }
    if(t.y >= eps)
    {
        VectorF p = ray.direction * t.y + (VectorF)ray.position;
        if(p.x < min.x || p.x > max.x || p.z < min.z || p.z > max.z)
            t.y = -1;
    }
    if(t.z >= eps)
    {
        VectorF p = ray.direction * t.z + (VectorF)ray.position;
        if(p.x < min.x || p.x > max.x || p.y < min.y || p.y > max.y)
            t.z = -1;
    }
    BoxRayCollision retval;
    if(t.x >= eps && (t.y < eps || t.y > t.x) && (t.z < eps || t.z > t.x)) // x is closest
    {
        retval.t = t.x;
        retval.enterFace = (ray.direction.x > 0) ? BlockFace::NX : BlockFace::PX;
    }
    else if(t.y >= eps && (t.z < eps || t.z > t.y)) // y is closest
    {
        retval.t = t.y;
        retval.enterFace = (ray.direction.y > 0) ? BlockFace::NY : BlockFace::PY;
    }
    else if(t.z >= eps) // z is closest
    {
        retval.t = t.z;
        retval.enterFace = (ray.direction.z > 0) ? BlockFace::NZ : BlockFace::PZ;
    }
    else if(ray.position.x > min.x && ray.position.x < max.x && ray.position.y > min.y && ray.position.y < max.y && ray.position.z > min.z && ray.position.z < max.z) // inside box
    {
        t = (collidePlane - (VectorF)ray.position) / ray.direction;
        retval.t = eps;
        if(t.x > t.z && t.x > t.y)
        {
            retval.enterFace = (ray.direction.x > 0) ? BlockFace::NX : BlockFace::PX;
        }
        else if(t.y > t.z)
        {
            retval.enterFace = (ray.direction.y > 0) ? BlockFace::NY : BlockFace::PY;
        }
        else
        {
            retval.enterFace = (ray.direction.z > 0) ? BlockFace::NZ : BlockFace::PZ;
        }
    }
    else
        return BoxRayCollision();
    collidePlane = min;
    if(ray.direction.x > 0)
        collidePlane.x = max.x;
    if(ray.direction.y > 0)
        collidePlane.y = max.y;
    if(ray.direction.z > 0)
        collidePlane.z = max.z;
    t = (collidePlane - (VectorF)ray.position) / ray.direction;
    if(t.x < t.y && t.x < t.z)
    {
        retval.exitFace = (ray.direction.x > 0) ? BlockFace::PX : BlockFace::NX;
    }
    else if(t.y < t.z)
    {
        retval.exitFace = (ray.direction.y > 0) ? BlockFace::PY : BlockFace::NY;
    }
    else
    {
        retval.exitFace = (ray.direction.z > 0) ? BlockFace::PZ : BlockFace::NZ;
    }
    return retval;
}

#endif // RAY_CASTING_H_INCLUDED
