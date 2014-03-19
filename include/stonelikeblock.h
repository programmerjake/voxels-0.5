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
#ifndef STONELIKEBLOCK_H
#define STONELIKEBLOCK_H

#include "block.h"
#include "block_face.h"
#include <iostream>

using namespace std;

class StoneLikeBlock : public BlockDescriptor
{
public:
    static inline RenderObjectBlockClass getStoneClass()
    {
        static RenderObjectBlockClass retval = 0;
        static bool didInit = false;
        if(!didInit)
        {
            didInit = true;
            retval = getRenderObjectBlockClass();
            //cout << "Stone Class : " << retval << endl;
        }
        return retval;
    }
protected:
    StoneLikeBlock(wstring name, LightProperties lightProperties = LightProperties(LightPropertiesType::Opaque, 0));
    virtual BlockData loadInternal(GameLoadStream & gls) const override;
    virtual void storeInternal(BlockData data, GameStoreStream & gss) const override;
    virtual shared_ptr<RenderObjectBlockMesh> getBlockMesh(BlockIterator bi) const override;
    virtual void onMove(BlockIterator) const override
    {
    }
    virtual shared_ptr<RenderObjectBlockMesh> internalMakeBlockMesh() const;
    virtual TextureDescriptor getFaceTexture(BlockFace face) const = 0;
    virtual Mesh makeBlockEntityMesh() const override;
    virtual shared_ptr<PhysicsObject> getPhysicsObject(PositionI pos) const override
    {
        return static_pointer_cast<PhysicsObject>(make_shared<PhysicsBox>((VectorI)pos + VectorF(0.5), VectorF(0.5), VectorF(0), VectorF(0), VectorF(0), pos.d, PhysicsProperties(PhysicsProperties::INFINITE_MASS, 0.8, 0.1)));
    }
private:
    mutable shared_ptr<RenderObjectBlockMesh> blockMesh;
};

#endif // STONELIKEBLOCK_H
