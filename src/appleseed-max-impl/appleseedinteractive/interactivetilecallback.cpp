
// Interface header.
#include "appleseedinteractive/interactivetilecallback.h"

namespace asr = renderer;

#define WM_TRIGGER_CALLBACK WM_USER + 4764

InteractiveTileCallback::InteractiveTileCallback(
    Bitmap*                         bitmap,
    IIRenderMgr*                    iimanager,
    asr::IRendererController*       render_controller
    )
    : TileCallback(bitmap, 0)
    , m_bitmap(bitmap)
    , m_iimanager(iimanager)
    , m_renderer_ctrl(render_controller)
{
}

void InteractiveTileCallback::update_window()
{
    if (m_iimanager->IsRendering())
        m_iimanager->UpdateDisplay();

    m_ui_promise.set_value();
}

void InteractiveTileCallback::update_caller(UINT_PTR param_ptr)
{
    InteractiveTileCallback* object_ptr = reinterpret_cast<InteractiveTileCallback*>(param_ptr);
    object_ptr->update_window();
}

void InteractiveTileCallback::on_progressive_frame_end(
    const asr::Frame* frame)
{
    TileCallback::on_progressive_frame_end(frame);

    //wait until ui proc gets handled to ensure class object is valid
    m_ui_promise = std::promise<void>();
    if (m_renderer_ctrl->get_status() == asr::IRendererController::ContinueRendering)
    {
        std::future<int> ui_future = m_ui_promise.get_future();
        post_callback(update_caller, (UINT_PTR)this);
        ui_future.wait();
    }
}

void InteractiveTileCallback::post_callback(void(*funcPtr)(UINT_PTR), UINT_PTR param)
{
  PostMessage(GetCOREInterface()->GetMAXHWnd(), WM_TRIGGER_CALLBACK, (UINT_PTR)funcPtr, (UINT_PTR)param);
}
