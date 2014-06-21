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
#include "stonelikeblock.h"
#include "generate.h"

#error finish changing to new physics engine

StoneLikeBlock::StoneLikeBlock(wstring name, LightProperties lightProperties)
    : BlockDescriptor(name, lightProperties)
{
}

BlockData StoneLikeBlock::loadInternal(GameLoadStream &) const
{
    return BlockData(shared_from_this());
}

void StoneLikeBlock::storeInternal(BlockData, GameStoreStream & ) const
{
    // empty on purpose
}

shared_ptr<RenderObjectBlockMesh> StoneLikeBlock::getBlockMesh(BlockIterator ) const
{
    if(blockMesh == nullptr)
        blockMesh = internalMakeBlockMesh();
    return blockMesh;
}

shared_ptr<RenderObjectBlockMesh> StoneLikeBlock::internalMakeBlockMesh() const
{
    return make_shared<RenderObjectBlockMesh>(getStoneClass(), VectorF(0), VectorF(1), LightProperties(LightPropertiesType::Opaque, 0), Mesh(new Mesh_t),
                Generate::unitBox(getFaceTexture(BlockFace::NX), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), getFaceTexture(BlockFace::PX), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureDescriptor(), getFaceTexture(BlockFace::NY), TextureDescriptor(), TextureDescriptor(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), getFaceTexture(BlockFace::PY), TextureDescriptor(), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), getFaceTexture(BlockFace::NZ), TextureDescriptor()),
                Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), TextureDescriptor(), getFaceTexture(BlockFace::PZ)),
                true, true, true, true, true, true, RenderLayer::Opaque, getPhysicsObjectConstructor()
                                                 );
}

Mesh StoneLikeBlock::makeBlockEntityMesh() const
{
    return Generate::unitBox(getFaceTexture(BlockFace::NX),
                             getFaceTexture(BlockFace::PX),
                             getFaceTexture(BlockFace::NY),
                             getFaceTexture(BlockFace::PY),
                             getFaceTexture(BlockFace::NZ),
                             getFaceTexture(BlockFace::PZ));
}
