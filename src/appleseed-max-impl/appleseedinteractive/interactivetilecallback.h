
#pragma once

// appleseed-max headers.
#include "appleseedrenderer/tilecallback.h"

// appleseed.foundation headers.
#include "foundation/image/tile.h"
#include "foundation/platform/types.h"
#include "foundation/platform/windows.h"    // include before 3ds Max headers

// 3ds Max headers.
#include "bitmap.h"
#include "interactiverender.h"

// Standard headers.
#include <future>

// Forward declarations.
namespace renderer  { class Frame; }
class Bitmap;
class IIRenderMgr;


class InteractiveTileCallback
  : public TileCallback
{
  public:
      InteractiveTileCallback(
        Bitmap*                         bitmap,
        IIRenderMgr*                    iimanager,
        renderer::IRendererController*  render_controller);

    virtual void on_progressive_frame_end(const renderer::Frame* frame) override;

    void update_window();
    void post_callback(void(*funcPtr)(UINT_PTR), UINT_PTR param);
    static void update_caller(UINT_PTR param_ptr);

  private:
    Bitmap*                         m_bitmap;
    IIRenderMgr*                    m_iimanager;
    renderer::IRendererController*  m_renderer_ctrl;
    std::promise<void>              m_ui_promise;
};
