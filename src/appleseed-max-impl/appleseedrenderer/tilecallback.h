
//
// This source file is part of appleseed.
// Visit https://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2015-2018 Francois Beaune, The appleseedhq Organization
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

#pragma once

// appleseed.renderer headers.
#include "renderer/api/rendering.h"

// appleseed.foundation headers.
#include "foundation/image/tile.h"
#include "foundation/platform/types.h"

// Standard headers.
#include <cstddef>
#include <memory>

// Forward declarations.
namespace renderer  { class Frame; }
class Bitmap;

class TileCallback
  : public renderer::TileCallbackBase
{
  public:
    TileCallback(
        Bitmap*                         bitmap,
        volatile foundation::uint32*    rendered_tile_count);

    void release() override;

    void on_tile_begin(
        const renderer::Frame*          frame,
        const size_t                    tile_x,
        const size_t                    tile_y) override;

    void on_tile_end(
        const renderer::Frame*          frame,
        const size_t                    tile_x,
        const size_t                    tile_y) override;

    void on_progressive_frame_update(
        const renderer::Frame*          frame) override;

  private:
    Bitmap*                             m_bitmap;
    volatile foundation::uint32*        m_rendered_tile_count;
    std::unique_ptr<foundation::Tile>   m_float_tile_storage;

    void blit_tile(
        const renderer::Frame&          frame,
        const size_t                    tile_x,
        const size_t                    tile_y);
};
