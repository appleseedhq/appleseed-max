
// Interface header.
#include "appleseedinteractive/interactivesession.h"

// appleseed-max headers.
#include "appleseedinteractive/interactiverenderercontroller.h"
#include "appleseedinteractive/interactivetilecallback.h"

// appleseed.renderer headers.
#include "renderer/api/project.h"
#include "renderer/api/rendering.h"

// 3ds Max headers.
#include "bitmap.h"
#include "interactiverender.h"


InteractiveSession::InteractiveSession(
    IIRenderMgr*                            iirender_mgr,
    asr::Project*                           project,
    const RendererSettings&                 settings,
    Bitmap*                                 bitmap
)
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
            (*m_project).configurations().get_by_name("interactive")->get_inherited_parameters(),
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
