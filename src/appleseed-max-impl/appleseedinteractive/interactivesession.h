
#pragma once

// appleseed-max headers.
#include "appleseedrenderer/renderersettings.h"

// appleseed.foundation headers.
#include "foundation/platform/windows.h"    // include before 3ds Max headers

// Standard headers.
#include <thread>
#include <memory>

// Forward declarations.
namespace renderer { class Project; }
class IRenderProgressCallback;
class IIRenderMgr;
class Bitmap;
class InteractiveRendererController;

namespace asf = foundation;
namespace asr = renderer;

class InteractiveSession
{
  public:
    InteractiveSession(
        IIRenderMgr*                iirender_mgr,
        asr::Project*               project,
        const RendererSettings&     settings,
        Bitmap*                     bitmap
    );

    void render_thread();
    void start_render();
    void abort_render();
    void reininitialize_render();
    void end_render();

  private:
    std::unique_ptr<InteractiveRendererController>  m_render_ctrl;
    std::thread                                     m_render_thread;
    Bitmap*                                         m_bitmap;
    IIRenderMgr*                                    m_iirender_mgr;
    asr::Project*                                   m_project;
    RendererSettings                                m_renderer_settings;
};
