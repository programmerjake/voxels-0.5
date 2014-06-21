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
#include "builtin_blocks.h"
#include "texture_atlas.h"
#include "generate.h"
#include "util.h"

#error finish changing to new physics engine

void initBuiltinBlocks()
{
    BlockDescriptor::initBlock(StoneBlock::ptr = shared_ptr<BlockDescriptor>(new StoneBlock));
    BlockDescriptor::initBlock(BedrockBlock::ptr = shared_ptr<BlockDescriptor>(new BedrockBlock));
    BlockDescriptor::initBlock(AirBlock::ptr = shared_ptr<BlockDescriptor>(new AirBlock));
    BlockDescriptor::initBlock(GlassBlock::ptr = shared_ptr<BlockDescriptor>(new GlassBlock));
    BlockDescriptor::initBlock(SandBlock::ptr = shared_ptr<BlockDescriptor>(new SandBlock));
}

namespace
{
initializer init1(&initBuiltinBlocks);
}

TextureDescriptor StoneBlock::getFaceTexture(BlockFace) const
{
    return TextureAtlas::Stone.td();
}

TextureDescriptor BedrockBlock::getFaceTexture(BlockFace) const
{
    return TextureAtlas::Bedrock.td();
}

shared_ptr<RenderObjectBlockMesh> AirBlock::makeBlockMesh() const
{
    RenderObjectBlockClass airClass = getRenderObjectBlockClass();
    //cout << "Air Class : " << airClass << endl;
    return make_shared<RenderObjectBlockMesh>(airClass, VectorF(0), VectorF(0),
            lightProperties, Mesh(new Mesh_t), Mesh(new Mesh_t),
            Mesh(new Mesh_t), Mesh(new Mesh_t), Mesh(new Mesh_t), Mesh(new Mesh_t), Mesh(new Mesh_t), false,
            false, false, false, false, false, RenderLayer::Opaque, getPhysicsObjectConstructor());
}

shared_ptr<RenderObjectBlockMesh> GlassBlock::makeBlockMesh() const
{
    RenderObjectBlockClass glassClass = getRenderObjectBlockClass();
    //cout << "Glass Class : " << airClass << endl;
    return make_shared<RenderObjectBlockMesh>(glassClass, VectorF(0), VectorF(1),
            lightProperties, Mesh(new Mesh_t),
            Generate::unitBox(TextureAtlas::Glass.td(), TextureDescriptor(), TextureDescriptor(),
                              TextureDescriptor(), TextureDescriptor(), TextureDescriptor()),
            Generate::unitBox(TextureDescriptor(), TextureAtlas::Glass.td(), TextureDescriptor(),
                              TextureDescriptor(), TextureDescriptor(), TextureDescriptor()),
            Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureAtlas::Glass.td(),
                              TextureDescriptor(), TextureDescriptor(), TextureDescriptor()),
            Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(),
                              TextureAtlas::Glass.td(), TextureDescriptor(), TextureDescriptor()),
            Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(),
                              TextureDescriptor(), TextureAtlas::Glass.td(), TextureDescriptor()),
            Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(),
                              TextureDescriptor(), TextureDescriptor(), TextureAtlas::Glass.td()),
            false, false, false, false, false, false, RenderLayer::Opaque, getPhysicsObjectConstructor()
                                             );
}

shared_ptr<RenderObjectBlockMesh> SandBlock::makeBlockMesh() const
{
    RenderObjectBlockClass sandClass = getRenderObjectBlockClass();
    return make_shared<RenderObjectBlockMesh>(sandClass, VectorF(0), VectorF(1),
            lightProperties, Mesh(new Mesh_t),
            Generate::unitBox(TextureAtlas::Sand.td(), TextureDescriptor(), TextureDescriptor(),
                              TextureDescriptor(), TextureDescriptor(), TextureDescriptor()),
            Generate::unitBox(TextureDescriptor(), TextureAtlas::Sand.td(), TextureDescriptor(),
                              TextureDescriptor(), TextureDescriptor(), TextureDescriptor()),
            Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureAtlas::Sand.td(),
                              TextureDescriptor(), TextureDescriptor(), TextureDescriptor()),
            Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(),
                              TextureAtlas::Sand.td(), TextureDescriptor(), TextureDescriptor()),
            Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(),
                              TextureDescriptor(), TextureAtlas::Sand.td(), TextureDescriptor()),
            Generate::unitBox(TextureDescriptor(), TextureDescriptor(), TextureDescriptor(),
                              TextureDescriptor(), TextureDescriptor(), TextureAtlas::Sand.td()),
            true, true, true, true, true, true, RenderLayer::Opaque, getPhysicsObjectConstructor()
                                             );
}

shared_ptr<BlockDescriptor> StoneBlock::ptr, BedrockBlock::ptr, AirBlock::ptr, GlassBlock::ptr, SandBlock::ptr;

Mesh GlassBlock::makeBlockEntityMesh() const
{
    return Generate::unitBox(TextureAtlas::Glass.td(),
                             TextureAtlas::Glass.td(),
                             TextureAtlas::Glass.td(),
                             TextureAtlas::Glass.td(),
                             TextureAtlas::Glass.td(),
                             TextureAtlas::Glass.td());
}

Mesh SandBlock::makeBlockEntityMesh() const
{
    return Generate::unitBox(TextureAtlas::Sand.td(),
                             TextureAtlas::Sand.td(),
                             TextureAtlas::Sand.td(),
                             TextureAtlas::Sand.td(),
                             TextureAtlas::Sand.td(),
                             TextureAtlas::Sand.td());
}
