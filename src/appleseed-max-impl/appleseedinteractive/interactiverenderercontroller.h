
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

// appleseed.renderer headers.
#include "renderer/api/project.h"
#include "renderer/api/rendering.h"
#include "renderer/api/scene.h"

// appleseed.foundation headers.
#include "foundation/utility/autoreleaseptr.h"

// appleseed-max headers.
#include "appleseedinteractive/appleseedinteractive.h"

// Standard headers.
#include <memory>
#include <vector>

// Forward declarations.
namespace renderer { class Camera; }
namespace renderer { class Project; }

class ScheduledAction
{
  public:
    virtual ~ScheduledAction() {}
    virtual void update() = 0;
};

class CameraObjectUpdateAction 
  : public ScheduledAction
{
  public:
    CameraObjectUpdateAction(
        renderer::Project&                              project,
        foundation::auto_release_ptr<renderer::Camera>  camera)
      : m_project(project)
      , m_camera(camera)
    {
    }

    virtual void update() override
    {
        m_project.get_scene()->cameras().clear();
        m_project.get_scene()->cameras().insert(m_camera);
    }

  public:
    foundation::auto_release_ptr<renderer::Camera>    m_camera;
    renderer::Project&                                m_project;
};

class InteractiveRendererController
  : public renderer::DefaultRendererController
{
  public:
    InteractiveRendererController();

    virtual void on_rendering_begin() override;
    virtual Status get_status() const override;

    void set_status(const Status status);

    void schedule_update(std::unique_ptr<ScheduledAction> updater);

  private:
    std::vector<std::unique_ptr<ScheduledAction>>   m_scheduled_actions;
    Status                                          m_status;
};
