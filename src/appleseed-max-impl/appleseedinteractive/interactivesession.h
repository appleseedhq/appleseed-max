
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

#pragma once

// appleseed-max headers.
#include "appleseedrenderer/renderersettings.h"

// Standard headers.
#include <memory>
#include <thread>

// Forward declarations.
namespace renderer { class Project; }
class Bitmap;
class IIRenderMgr;
class InteractiveRendererController;
class RendProgressCallback;
class IInteractiveRender;

class InteractiveSession
{
  public:
    InteractiveSession(
        IIRenderMgr*                iirender_mgr,
        IInteractiveRender*         renderer,
        RendProgressCallback*       progress_cb,
        renderer::Project*          project,
        const RendererSettings&     settings,
        Bitmap*                     bitmap);

    void start_render();
    void abort_render();
    void reininitialize_render();
    void end_render();

  private:
    void render_thread();

    RendProgressCallback*                           m_progress_cb;
    std::unique_ptr<InteractiveRendererController>  m_render_ctrl;
    std::thread                                     m_render_thread;
    Bitmap*                                         m_bitmap;
    IIRenderMgr*                                    m_iirender_mgr;
    IInteractiveRender*                             m_renderer;
    renderer::Project*                              m_project;
    RendererSettings                                m_renderer_settings;
};
