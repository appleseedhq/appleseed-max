
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2015-2017 Francois Beaune, The appleseedhq Organization
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

// Interface header.
#include "tilecallback.h"

// appleseed.renderer headers.
#include "renderer/api/frame.h"

// appleseed.foundation headers.
#include "foundation/image/canvasproperties.h"
#include "foundation/image/color.h"
#include "foundation/image/image.h"
#include "foundation/image/pixel.h"
#include "foundation/platform/atomic.h"
#include "foundation/platform/windows.h"    // include before 3ds Max headers

// 3ds Max headers.
#include <assert1.h>
#include <bitmap.h>

// Standard headers.
#include <algorithm>

namespace asf = foundation;
namespace asr = renderer;

namespace
{
    void draw_hline(
        Bitmap*             bitmap,
        const int           x,
        const int           y,
        const int           length,
        BMM_Color_fl*       pixel)
    {
        const int s = length > 0 ? 1 : -1;
        for (int i = x; i != x + length; i += s)
            bitmap->PutPixels(i, y, 1, pixel);
    }

    void draw_vline(
        Bitmap*             bitmap,
        const int           x,
        const int           y,
        const int           length,
        BMM_Color_fl*       pixel)
    {
        const int s = length > 0 ? 1 : -1;
        for (int i = y; i != y + length; i += s)
            bitmap->PutPixels(x, i, 1, pixel);
    }

    void draw_bracket(
        Bitmap*             bitmap,
        const int           x,
        const int           y,
        const int           width,
        const int           height,
        const int           bracket_extent,
        BMM_Color_fl*       pixel)
    {
        const int w = static_cast<int>(std::min(bracket_extent, width));
        const int h = static_cast<int>(std::min(bracket_extent, height));

        // Top-left corner.
        draw_hline(bitmap, x, y, w, pixel);
        draw_vline(bitmap, x, y, h, pixel);

        // Top-right corner.
        draw_hline(bitmap, x + width - 1, y, -w, pixel);
        draw_vline(bitmap, x + width - 1, y, h, pixel);

        // Bottom-left corner.
        draw_hline(bitmap, x, y + height - 1, w, pixel);
        draw_vline(bitmap, x, y + height - 1, -h, pixel);

        // Bottom-right corner.
        draw_hline(bitmap, x + width - 1, y + height - 1, -w, pixel);
        draw_vline(bitmap, x + width - 1, y + height - 1, -h, pixel);
    }

    RECT make_rect(
        const size_t        x,
        const size_t        y,
        const size_t        width,
        const size_t        height)
    {
        RECT rect;
        rect.left = static_cast<LONG>(x);
        rect.top = static_cast<LONG>(y);
        rect.right = static_cast<LONG>(x + width);
        rect.bottom = static_cast<LONG>(y + height);
        return rect;
    }
}

TileCallback::TileCallback(
    Bitmap*                 bitmap,
    volatile asf::uint32*   rendered_tile_count)
  : m_bitmap(bitmap)
  , m_rendered_tile_count(rendered_tile_count)
{
}

void TileCallback::release()
{
    delete this;
}

void TileCallback::on_tile_begin(
    const asr::Frame*       frame,
    const size_t            tile_x,
    const size_t            tile_y)
{
    const asf::Image& image = frame->image();
    const asf::CanvasProperties& props = image.properties();

    DbgAssert(props.m_canvas_width == m_bitmap->Width());
    DbgAssert(props.m_canvas_height == m_bitmap->Height());
    DbgAssert(props.m_channel_count == 4);

    const asf::Tile& tile = image.tile(tile_x, tile_y);
    const size_t x = tile_x * props.m_tile_width;
    const size_t y = tile_y * props.m_tile_height;

    // Draw a bracket around the tile.
    const int BracketExtent = 5;
    BMM_Color_fl BracketColor(1.0f, 1.0f, 1.0f, 1.0f);
    draw_bracket(
        m_bitmap,
        static_cast<int>(x),
        static_cast<int>(y),
        static_cast<int>(tile.get_width()),
        static_cast<int>(tile.get_height()),
        BracketExtent,
        &BracketColor);

    // Partially refresh the display window.
    RECT rect = make_rect(x, y, tile.get_width(), tile.get_height());
    m_bitmap->RefreshWindow(&rect);
}

void TileCallback::on_tile_end(
    const asr::Frame*       frame,
    const size_t            tile_x,
    const size_t            tile_y)
{
    const asf::Image& image = frame->image();
    const asf::CanvasProperties& props = image.properties();

    DbgAssert(props.m_canvas_width == m_bitmap->Width());
    DbgAssert(props.m_canvas_height == m_bitmap->Height());
    DbgAssert(props.m_channel_count == 4);

    const asf::Tile& tile = image.tile(tile_x, tile_y);
    const size_t x = tile_x * props.m_tile_width;
    const size_t y = tile_y * props.m_tile_height;

    // Blit the tile to the destination bitmap.
    blit_tile(*frame, tile_x, tile_y);

    // Partially refresh the display window.
    RECT rect = make_rect(x, y, tile.get_width(), tile.get_height());
    m_bitmap->RefreshWindow(&rect);

    // Keep track of the number of rendered tiles.
    asf::atomic_inc(m_rendered_tile_count);
}

void TileCallback::on_progressive_frame_end(
    const asr::Frame*       frame)
{
    const asf::CanvasProperties& props = frame->image().properties();

    DbgAssert(props.m_canvas_width == m_bitmap->Width());
    DbgAssert(props.m_canvas_height == m_bitmap->Height());
    DbgAssert(props.m_channel_count == 4);

    // Blit all tiles.
    for (size_t y = 0; y < props.m_tile_count_y; ++y)
    {
        for (size_t x = 0; x < props.m_tile_count_x; ++x)
            blit_tile(*frame, x, y);
    }

    // Refresh the entire display window.
    m_bitmap->RefreshWindow();
}

void TileCallback::blit_tile(
    const asr::Frame&       frame,
    const size_t            tile_x,
    const size_t            tile_y)
{
    const asf::CanvasProperties& props = frame.image().properties();

    // Allocate memory for the temporary tile.
    if (m_float_tile_storage.get() == nullptr)
    {
        m_float_tile_storage.reset(
            new asf::Tile(
                props.m_tile_width,
                props.m_tile_height,
                props.m_channel_count,
                asf::PixelFormatFloat));
    }

    // Retrieve the source tile.
    const asf::Tile& tile = frame.image().tile(tile_x, tile_y);

    // Convert the tile to 32-bit floating point.
    asf::Tile fp_tile(
        tile,
        asf::PixelFormatFloat,
        m_float_tile_storage->get_storage());

    // Transform the tile to the color space of the frame.
    frame.transform_to_output_color_space(fp_tile);

    // Blit the time into the bitmap.
    const size_t dest_x = tile_x * props.m_tile_width;
    const size_t dest_y = tile_y * props.m_tile_height;
    const size_t tile_width = fp_tile.get_width();
    const size_t tile_height = fp_tile.get_height();
    for (size_t y = 0; y < tile_height; ++y)
    {
        for (size_t x = 0; x < tile_width; ++x)
        {
            static_assert(
                sizeof(BMM_Color_fl) == sizeof(asf::Color4f),
                "BMM_Color_fl is expected to be the same size of foundation::Color4f");

            asf::Color4f color;
            fp_tile.get_pixel(x, y, &color.r);

            m_bitmap->PutPixels(
                static_cast<int>(dest_x + x),
                static_cast<int>(dest_y + y),
                1,
                reinterpret_cast<BMM_Color_fl*>(&color));
        }
    }
}
