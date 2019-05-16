
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
#include "appleseedinteractive/interactiverenderercontroller.h"
#include "appleseedrenderer/renderersettings.h"

// Build options header.
#include "renderer/api/buildoptions.h"

// appleseed.foundation headers.
#include "foundation/utility/autoreleaseptr.h"

// Standard headers.
#include <memory>
#include <thread>

// Forward declarations.
namespace renderer { class Camera; }
namespace renderer { class Project; }
class Bitmap;
class IIRenderMgr;

class InteractiveSession
{
  public:
    InteractiveSession(
        IIRenderMgr*                iirender_mgr,
        renderer::Project*          project,
        const RendererSettings&     settings,
        Bitmap*                     bitmap);

    void start_render();
    void abort_render();
    void reininitialize_render();
    void end_render();

    void schedule_camera_update(
        foundation::auto_release_ptr<renderer::Camera>  camera);

  private:
    std::unique_ptr<InteractiveRendererController>  m_renderer_controller;
    std::thread                                     m_render_thread;
    Bitmap*                                         m_bitmap;
    IIRenderMgr*                                    m_iirender_mgr;
    renderer::Project*                              m_project;
    RendererSettings                                m_renderer_settings;

    void render_thread();
};
