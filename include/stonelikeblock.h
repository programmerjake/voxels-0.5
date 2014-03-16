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
    StoneLikeBlock(wstring name);
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
        return static_pointer_cast<PhysicsObject>(make_shared<PhysicsBox>((VectorI)pos + VectorF(0.5), VectorF(0.5), pos.d, PhysicsProperties(PhysicsProperties::INFINITE_MASS, 0.8, 0.1)));
    }
private:
    mutable shared_ptr<RenderObjectBlockMesh> blockMesh;
};

#endif // STONELIKEBLOCK_H
