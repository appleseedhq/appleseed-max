
// Interface header.
#include "interactiverenderercontroller.h"

namespace asr = renderer;

InteractiveRendererController::InteractiveRendererController()
  : m_status(ContinueRendering)
{
}

void InteractiveRendererController::on_rendering_begin()
{
    m_status = ContinueRendering;
}

asr::IRendererController::Status InteractiveRendererController::get_status() const
{
    return m_status;
}

void InteractiveRendererController::set_status(const Status status)
{
    m_status = status;
}
