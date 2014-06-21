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
#include "world.h"
#include "platform.h"
#include "render_layer.h"
#include "builtin_blocks.h"
#include <memory>

#error finish changing to new physics engine

using namespace std;

void renderLayerSetup(RenderLayer rl)
{
    switch(rl)
    {
    case RenderLayer::Opaque:
    case RenderLayer::Last:
        glDepthMask(GL_TRUE);
        break;

    case RenderLayer::Translucent:
        glDepthMask(GL_FALSE);
        break;

    default:
        break;
    }
}

BlockData BlockIterator::makeBedrock()
{
    return BlockData(BedrockBlock::ptr);
}

BlockData BlockIterator::makeLitAir()
{
    BlockData retval(AirBlock::ptr);
    retval.light = Lighting::sky();
    return retval;
}

