
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2015 Francois Beaune, The appleseedhq Organization
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
#include "foundation/platform/types.h"

// Standard headers.
#include <cstddef>

// Forward declarations.
class RendProgressCallback;

class RendererController
  : public renderer::DefaultRendererController
{
  public:
    RendererController(
        RendProgressCallback*   progress_cb,
        foundation::uint32*     rendered_tile_count,
        const size_t            total_tile_count);

    virtual void on_rendering_begin() override;

    virtual void on_progress() override;

    virtual Status get_status() const override;

  private:
    RendProgressCallback*       m_progress_cb;
    foundation::uint32*         m_rendered_tile_count;
    const size_t                m_total_tile_count;
    Status                      m_status;
};
