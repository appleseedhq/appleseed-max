
#pragma once

// appleseed.renderer headers.
#include "renderer/api/rendering.h"


class InteractiveRendererController
  : public renderer::DefaultRendererController
{
  public:
    InteractiveRendererController();

    virtual void on_rendering_begin() override;
    virtual Status get_status() const override;
    
    void set_status(const Status status);

  private:
    Status m_status;
};
