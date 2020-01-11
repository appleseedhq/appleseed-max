
//
// This source file is part of appleseed.
// Visit https://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2017-2018 Sergo Pogosyan, The appleseedhq Organization
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

// appleseed-max headers.
#include "appleseedrenderer/tilecallback.h"

// Build options header.
#include "foundation/core/buildoptions.h"

// appleseed.foundation headers.
#include "foundation/platform/types.h"
#include "foundation/platform/windows.h"

// Standard headers.
#include <cstdint>
#include <future>

// Forward declarations.
namespace renderer  { class Frame; }
namespace renderer  { class IRendererController; }
class Bitmap;
class IIRenderMgr;

class InteractiveTileCallback
  : public TileCallback
{
  public:
    InteractiveTileCallback(
        Bitmap*                         bitmap,
        IIRenderMgr*                    irender_manager,
        renderer::IRendererController*  renderer_controller);

    void on_progressive_frame_update(
        const renderer::Frame&          frame,
        const double                    time,
        const std::uint64_t             samples,
        const double                    samples_per_pixel,
        const std::uint64_t             samples_per_second) override;

  private:
    Bitmap*                             m_bitmap;
    IIRenderMgr*                        m_irender_manager;
    renderer::IRendererController*      m_renderer_controller;
    std::promise<void>                  m_ui_promise;

    static void update_caller(UINT_PTR param_ptr);
};
