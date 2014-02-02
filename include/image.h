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
#include <mutex>
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
    const Image &operator =(const Image &);
    ~Image();

    void setPixel(int x, int y, Color c);
    Color getPixel(int x, int y) const;
    void bind();
    static void unbind();
    unsigned width() const
    {
        return data->w;
    }
    unsigned height() const
    {
        return data->h;
    }
private:
    enum RowOrder
    {
        TopToBottom,
        BottomToTop
    };
    struct data_t
    {
        uint8_t * const data;
        unsigned refcount;
        const unsigned w, h;
        RowOrder rowOrder;
        uint32_t texture;
        bool textureValid;
        mutex lock;
        data_t(uint8_t * data, unsigned w, unsigned h, RowOrder rowOrder)
            : data(data), refcount(0), w(w), h(h), rowOrder(rowOrder), texture(0), textureValid(false), lock()
        {
        }
        data_t(uint8_t * data, data_t * rt)
            : data(data), refcount(0), w(rt->w), h(rt->h), rowOrder(rt->rowOrder), texture(0), textureValid(false), lock()
        {
        }
        ~data_t()
        {
            delete []data;
        }
    };
    data_t *data;
    static constexpr size_t BytesPerPixel = 4;
    void setRowOrder(RowOrder newRowOrder);
    void swapRows(unsigned y1, unsigned y2);
    void copyOnWrite();
};

#endif // IMAGE_H
