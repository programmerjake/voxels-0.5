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
#ifndef IMAGE_H
#define IMAGE_H

#include <cstdint>
#include <cwchar>
#include <string>
#include <stdexcept>
#include "color.h"

using namespace std;

class ImageLoadError final : public runtime_error
{
public:
    explicit ImageLoadError(const string &arg)
        : runtime_error(arg)
    {
    }
};

class Image final
{
public:
    explicit Image(wstring resourceName);
    explicit Image(unsigned w, unsigned h);
    explicit Image(Color c);
    Image(const Image &rt);
    Image(Image &&rt);
    ~Image();

    void setPixel(int x, int y, Color c);
    Color getPixel(int x, int y) const;
    void bind();
    static void unbind();
    unsigned width() const
    {
        return w;
    }
    unsigned height() const
    {
        return h;
    }
    Image *dup() const
    {
        return new Image(*this);
    }
private:
    const Image &operator =(const Image &) = delete;
    uint8_t *data;
    unsigned w, h;
    enum {BytesPerPixel = 4};
    enum RowOrder
    {
        TopToBottom,
        BottomToTop
    };
    RowOrder rowOrder;
    uint32_t texture;
    bool textureValid;
    void setRowOrder(RowOrder newRowOrder);
    void swapRows(int y1, int y2);
};

#endif // IMAGE_H
