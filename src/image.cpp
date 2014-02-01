#include "image.h"
#include "png_decoder.h"
#include "platform.h"
#include <cstring>

Image::Image(string resourceName)
{
    try
    {
        PngDecoder decoder(getResourceFileName(resourceName));
        w = decoder.width();
        h = decoder.height();
        rowOrder = TopToBottom;
        texture = 0;
        textureValid = false;
        data = decoder.removeData();
    }
    catch(PngLoadError *e)
    {
        ImageLoadError *e2 = new ImageLoadError(e->what());
        delete e;
        throw e2;
    }
}

Image::Image(unsigned w, unsigned h)
{
    this->w = w;
    this->h = h;
    rowOrder = TopToBottom;
    texture = 0;
    textureValid = false;
    data = new uint8_t[BytesPerPixel * w * h];
    memset((void *)data, 0, BytesPerPixel * w * h);
}

Image::Image(Color c)
{
    w = h = 1;
    rowOrder = TopToBottom;
    texture = 0;
    textureValid = false;
    data = new uint8_t[BytesPerPixel * w * h];
    setPixel(0, 0, c);
}

Image::Image(const Image &rt)
{
    w = rt.w;
    h = rt.h;
    rowOrder = rt.rowOrder;
    texture = 0;
    textureValid = false;
    data = new uint8_t[BytesPerPixel * w * h];
    memcpy((void *)data, (const void *)rt.data, BytesPerPixel * w * h);
}

Image::Image(Image &&rt)
{
    w = rt.w;
    h = rt.h;
    rowOrder = rt.rowOrder;
    texture = rt.texture;
    textureValid = rt.textureValid;
    data = rt.data;
    rt.w = 1;
    rt.h = 1;
    rt.texture = 0;
    rt.textureValid = false;
    rt.data = new uint8_t[BytesPerPixel];
    memset((void *)rt.data, 0, BytesPerPixel);
}

Image::~Image()
{
    static_assert(sizeof(uint32_t) == sizeof(GLuint), "GLuint is not the same size as uint32_t");
    if(texture != 0)
    {
        glDeleteTextures(1, (const GLuint *)&texture);
    }
    delete []data;
}

void Image::setPixel(int x, int y, Color c)
{
    if(rowOrder == BottomToTop)
    {
        y = h - y - 1;
    }
    if(y < 0 || (unsigned)y >= h || x < 0 || (unsigned)x >= w)
    {
        return;
    }
    uint8_t *pixel = &data[BytesPerPixel * (x + y * w)];
    pixel[0] = c.ri();
    pixel[1] = c.gi();
    pixel[2] = c.bi();
    pixel[3] = c.ai();
}

Color Image::getPixel(int x, int y) const
{
    Color retval;
    if(rowOrder == BottomToTop)
    {
        y = h - y - 1;
    }
    if(y < 0 || (unsigned)y >= h || x < 0 || (unsigned)x >= w)
    {
        return Color();
    }
    uint8_t *pixel = &data[BytesPerPixel * (x + y * w)];
    retval.ri(pixel[0]);
    retval.gi(pixel[0]);
    retval.bi(pixel[0]);
    retval.ai(pixel[0]);
    return retval;
}

void Image::bind()
{
    static_assert(sizeof(uint32_t) == sizeof(GLuint), "GLuint is not the same size as uint32_t");
    setRowOrder(BottomToTop);
    if(textureValid)
    {
        glBindTexture(GL_TEXTURE_2D, texture);
        return;
    }
    if(texture == 0)
    {
        glGenTextures(1, (GLuint *)&texture);
    }
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelTransferf(GL_ALPHA_SCALE, 1.0);
    glPixelTransferf(GL_ALPHA_BIAS, 0.0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid *)data);
    textureValid = true;
}

void Image::unbind()
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Image::setRowOrder(RowOrder newRowOrder)
{
    if(rowOrder == newRowOrder)
    {
        return;
    }
    rowOrder = newRowOrder;
    for(int y1 = 0, y2 = h - 1; y1 < y2; y1++, y2--)
    {
        swapRows(y1, y2);
    }
}

void Image::swapRows(int y1, int y2)
{
    int index1 = y1 * w * BytesPerPixel, index2 = y2 * w * BytesPerPixel;
    for(unsigned i = 0; i < w * BytesPerPixel; i++)
    {
        uint8_t t = data[index1];
        data[index1++] = data[index2];
        data[index2++] = t;
    }
}

