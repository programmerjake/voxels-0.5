#ifndef TEXTURE_DESCRIPTOR_H_INCLUDED
#define TEXTURE_DESCRIPTOR_H_INCLUDED

#include "image.h"
#include "util.h"

struct TextureDescriptor
{
    Image image;
    float minU, maxU, minV, maxV;
    TextureDescriptor(Image image = Image())
        : image(image), minU(0), maxU(1), minV(0), maxV(1)
    {
    }
    TextureDescriptor(Image image, float minU, float maxU, float minV, float maxV)
        : image(image), minU(minU), maxU(maxU), minV(minV), maxV(maxV)
    {
    }
    operator bool() const
    {
        return (bool)image;
    }
    bool operator !() const
    {
        return !image;
    }
    TextureDescriptor subTexture(const float minU, const float maxU, const float minV, const float maxV) const
    {
        return TextureDescriptor(image,
                                 interpolate(minU, this->minU, this->maxU),
                                 interpolate(maxU, this->minU, this->maxU),
                                 interpolate(minV, this->minV, this->maxV),
                                 interpolate(maxV, this->minV, this->maxV));
    }
};

#endif // TEXTURE_DESCRIPTOR_H_INCLUDED
