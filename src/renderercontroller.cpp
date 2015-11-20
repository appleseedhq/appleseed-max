
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

// Interface header.
#include "renderercontroller.h"

// appleseed.foundation headers.
#include "foundation/platform/thread.h"
#include "foundation/platform/windows.h"    // include before 3ds Max headers

// 3ds Max headers.
#include <render.h>

namespace asf = foundation;
namespace asr = renderer;

RendererController::RendererController(RendProgressCallback* progress_cb)
  : m_progress_cb(progress_cb)
  , m_status(ContinueRendering)
{
}

void RendererController::on_rendering_begin()
{
    m_status = ContinueRendering;
}

void RendererController::on_progress()
{
    m_status =
        m_progress_cb->Progress(0, 10) == RENDPROG_CONTINUE
            ? ContinueRendering
            : AbortRendering;

    asf::sleep(100);
}

asr::IRendererController::Status RendererController::get_status() const
{
    return m_status;
}
