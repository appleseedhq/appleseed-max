
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

// Interface header.
#include "interactivetilecallback.h"

// 3ds Max headers.
#include <bitmap.h>
#include <interactiverender.h>
#include <maxapi.h>

namespace asr = renderer;


//
// InteractiveTileCallback class implementation.
//

namespace
{
    const UINT WM_TRIGGER_CALLBACK = WM_USER + 4764;
}

InteractiveTileCallback::InteractiveTileCallback(
    Bitmap*                     bitmap,
    IIRenderMgr*                iimanager,
    asr::IRendererController*   renderer_controller)
  : TileCallback(bitmap, nullptr)
  , m_bitmap(bitmap)
  , m_iimanager(iimanager)
  , m_renderer_controller(renderer_controller)
{
}

void InteractiveTileCallback::on_progressive_frame_update(
    const asr::Frame*           frame)
{
    TileCallback::on_progressive_frame_update(frame);

    // Wait until UI proc gets handled to ensure class object is valid.
    m_ui_promise = std::promise<void>();
    if (m_renderer_controller->get_status() == asr::IRendererController::ContinueRendering)
    {
        PostMessage(
            GetCOREInterface()->GetMAXHWnd(),
            WM_TRIGGER_CALLBACK,
            reinterpret_cast<UINT_PTR>(update_caller),
            reinterpret_cast<UINT_PTR>(this));
        m_ui_promise.get_future().wait();
    }
}

void InteractiveTileCallback::update_caller(UINT_PTR param_ptr)
{
    auto tile_callback = reinterpret_cast<InteractiveTileCallback*>(param_ptr);
    
    if (tile_callback->m_iimanager->IsRendering())
        tile_callback->m_iimanager->UpdateDisplay();

    tile_callback->m_ui_promise.set_value();
}


//
// InteractiveTileCallbackFactory class implementation.
//

InteractiveTileCallbackFactory::InteractiveTileCallbackFactory(
    Bitmap*                     bitmap,
    IIRenderMgr*                iimanager,
    asr::IRendererController*   renderer_controller)
  : m_bitmap(bitmap)
  , m_iimanager(iimanager)
  , m_renderer_controller(renderer_controller)
{
}

void InteractiveTileCallbackFactory::release()
{
    delete this;
}

asr::ITileCallback* InteractiveTileCallbackFactory::create()
{
    return new InteractiveTileCallback(m_bitmap, m_iimanager, m_renderer_controller);
}
