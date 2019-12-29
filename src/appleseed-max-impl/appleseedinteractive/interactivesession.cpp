
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
#include "interactivesession.h"

// appleseed-max headers.
#include "appleseedinteractive/interactivetilecallback.h"

// appleseed-max-common headers.
#include "appleseed-max-common/iappleseedmtl.h"

// Build options header.
#include "foundation/core/buildoptions.h"

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
  , m_renderer_controller(nullptr)
{
}

void InteractiveSession::render_thread()
{
    // Create the renderer controller.
    m_renderer_controller.reset(new InteractiveRendererController());

    // Create the tile callback.
    InteractiveTileCallback m_tile_callback(
        m_bitmap,
        m_iirender_mgr,
        m_renderer_controller.get());

    // Create the master renderer.
    std::unique_ptr<asr::MasterRenderer> renderer(
        new asr::MasterRenderer(
            *m_project,
            m_project->configurations().get_by_name("interactive")->get_inherited_parameters(),
            m_search_paths,
            &m_tile_callback));

    // Render the frame.
    renderer->render(*m_renderer_controller);
}

void InteractiveSession::start_render()
{
    m_render_thread = std::thread(&InteractiveSession::render_thread, this);
}

void InteractiveSession::abort_render()
{
    m_renderer_controller->set_status(asr::IRendererController::AbortRendering);
}

void InteractiveSession::reininitialize_render()
{
    m_renderer_controller->set_status(asr::IRendererController::ReinitializeRendering);
}

void InteractiveSession::end_render()
{
    if (m_render_thread.joinable())
        m_render_thread.join();
}

void InteractiveSession::schedule_camera_update(
    asf::auto_release_ptr<asr::Camera>  camera)
{
    m_renderer_controller->schedule_update(
        std::unique_ptr<ScheduledAction>(new CameraObjectUpdateAction(*m_project, camera)));
}

void InteractiveSession::schedule_material_update(const IAppleseedMtlMap& material_map)
{
    m_renderer_controller->schedule_update(
        std::unique_ptr<ScheduledAction>(new MaterialUpdateAction(*m_project, material_map)));
}

void InteractiveSession::schedule_assign_material(
    const IAppleseedMtlMap& material_map, const InstanceMap& instances)
{
    m_renderer_controller->schedule_update(
        std::unique_ptr<ScheduledAction>(new AssignMaterialAction(*m_project, material_map, instances)));
}
