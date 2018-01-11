
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2017 Sergo Pogosyan, The appleseedhq Organization
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
#include "interactivesession.h"

// appleseed-max headers.
#include "appleseedinteractive/interactiverenderercontroller.h"
#include "appleseedinteractive/interactivetilecallback.h"

// appleseed.renderer headers.
#include "renderer/api/project.h"
#include "renderer/api/rendering.h"

namespace asf = foundation;
namespace asr = renderer;

InteractiveSession::InteractiveSession(
    IIRenderMgr*                iirender_mgr,
    asr::Project*               project,
    const RendererSettings&     settings,
    Bitmap*                     bitmap)
  : m_project(project)
  , m_iirender_mgr(iirender_mgr)
  , m_renderer_settings(settings)
  , m_bitmap(bitmap)
  , m_render_ctrl(nullptr)
{
}

void InteractiveSession::render_thread()
{
    // Create the renderer controller.
    m_render_ctrl.reset(new InteractiveRendererController());

    // Create the tile callback.
    InteractiveTileCallback m_tile_callback(m_bitmap, m_iirender_mgr, m_render_ctrl.get());

    // Create the master renderer.
    std::auto_ptr<asr::MasterRenderer> renderer(
        new asr::MasterRenderer(
            *m_project,
            m_project->configurations().get_by_name("interactive")->get_inherited_parameters(),
            m_render_ctrl.get(),
            &m_tile_callback));

    // Render the frame.
    renderer->render();
}

void InteractiveSession::start_render()
{
    m_render_thread = std::thread(&InteractiveSession::render_thread, this);
}

void InteractiveSession::abort_render()
{
    m_render_ctrl->set_status(asr::IRendererController::AbortRendering);
}

void InteractiveSession::reininitialize_render()
{
    m_render_ctrl->set_status(asr::IRendererController::ReinitializeRendering);
}

void InteractiveSession::end_render()
{
    if (m_render_thread.joinable())
        m_render_thread.join();
}

InteractiveRendererController* InteractiveSession::get_render_controller()
{
    return m_render_ctrl.get();
}
