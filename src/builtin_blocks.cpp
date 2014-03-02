#include "builtin_blocks.h"
#include "texture_atlas.h"
#include "generate.h"
#include "util.h"

void initBuiltinBlocks()
{
    BlockDescriptor::initBlock(StoneBlock::ptr = shared_ptr<BlockDescriptor>(new StoneBlock));
    BlockDescriptor::initBlock(BedrockBlock::ptr = shared_ptr<BlockDescriptor>(new BedrockBlock));
    BlockDescriptor::initBlock(AirBlock::ptr = shared_ptr<BlockDescriptor>(new AirBlock));
    BlockDescriptor::initBlock(GlassBlock::ptr = shared_ptr<BlockDescriptor>(new GlassBlock));
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

shared_ptr<RenderObjectBlockMesh> AirBlock::makeBlockMesh()
{
    RenderObjectBlockClass airClass = getRenderObjectBlockClass();
    //cout << "Air Class : " << airClass << endl;
    return make_shared<RenderObjectBlockMesh>(airClass,
            LightProperties(LightPropertiesType::Transparent, 0), Mesh(new Mesh_t), Mesh(new Mesh_t),
            Mesh(new Mesh_t), Mesh(new Mesh_t), Mesh(new Mesh_t), Mesh(new Mesh_t), Mesh(new Mesh_t), false,
            false, false, false, false, false, RenderLayer::Opaque);
}

shared_ptr<RenderObjectBlockMesh> GlassBlock::makeBlockMesh()
{
    RenderObjectBlockClass glassClass = getRenderObjectBlockClass();
    //cout << "Glass Class : " << airClass << endl;
    return make_shared<RenderObjectBlockMesh>(glassClass,
            LightProperties(LightPropertiesType::Transparent, 0), Mesh(new Mesh_t),
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
            false, false, false, false, false, false, RenderLayer::Opaque
                                             );
}

shared_ptr<BlockDescriptor> StoneBlock::ptr, BedrockBlock::ptr, AirBlock::ptr, GlassBlock::ptr;
