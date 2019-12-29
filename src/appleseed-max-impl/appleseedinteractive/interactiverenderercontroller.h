
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
#include "appleseedinteractive/appleseedinteractive.h"
#include "appleseedrenderer/projectbuilder.h"
#include "utilities.h"

// appleseed-max-common headers.
#include "appleseed-max-common/iappleseedmtl.h"

// Build options header.
#include "foundation/core/buildoptions.h"

// appleseed.renderer headers.
#include "renderer/api/project.h"
#include "renderer/api/rendering.h"
#include "renderer/api/scene.h"

// appleseed.foundation headers.
#include "foundation/utility/autoreleaseptr.h"

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

    void update() override
    {
        m_project.get_scene()->cameras().clear();
        m_project.get_scene()->cameras().insert(m_camera);
    }

  public:
    foundation::auto_release_ptr<renderer::Camera>    m_camera;
    renderer::Project&                                m_project;
};

class MaterialUpdateAction
  : public ScheduledAction
{
  public:
    MaterialUpdateAction(
        renderer::Project&      project,
        const IAppleseedMtlMap& material_map)
      : m_material_map(material_map)
      , m_project(project)
    {
    }

    void update() override
    {
        renderer::Assembly* assembly = m_project.get_scene()->assemblies().get_by_name("assembly");
        for (const auto& mtl : m_material_map)
        {
            renderer::Material* material = assembly->materials().get_by_name(mtl.second.c_str());

            if (material)
                assembly->materials().remove(material);

            assembly->materials().insert(
                mtl.first->create_material(
                *assembly,
                mtl.second.c_str(),
                false,
                GetCOREInterface()->GetTime()));
        }
    }

  private:
    IAppleseedMtlMap        m_material_map;
    renderer::Project&      m_project;
};

class AssignMaterialAction
  : public ScheduledAction
{
  public:
    AssignMaterialAction(
        renderer::Project&      project,
        const MaterialMap&      material_map,
        const InstanceMap&      instances)
      : m_material_map(material_map)
      , m_project(project)
      , m_instances(instances)
    {
    }

    void update() override
    {
        renderer::Assembly* assembly = m_project.get_scene()->assemblies().get_by_name("assembly");
        for (const auto& mtl : m_material_map)
        {
            renderer::Material* material = assembly->materials().get_by_name(mtl.second.c_str());

            if (material == nullptr)
            {
                const auto mtl_name =
                    make_unique_name(assembly->materials(), wide_to_utf8(mtl.first->GetName()) + "_mat");

                IAppleseedMtl* appleseed_mtl =
                    static_cast<IAppleseedMtl*>(mtl.first->GetInterface(IAppleseedMtl::interface_id()));
                if (appleseed_mtl == nullptr)
                    continue;

                const size_t index = assembly->materials().insert(
                    appleseed_mtl->create_material(
                    *assembly,
                    mtl_name.c_str(),
                    false,
                    GetCOREInterface()->GetTime()));
                material = assembly->materials().get_by_index(index);
            }

            for (const auto& obj_instance : m_instances)
            {
                auto* instance = obj_instance.first;
                auto& obj_info = obj_instance.second;

                auto front_mapping = instance->get_front_material_mappings();
                for (const auto& mat_mapping : front_mapping)
                {
                    // TODO: recognize multi materials
                    auto test1 = mat_mapping.key();
                    auto test2 = mat_mapping.value();
                    instance->assign_material(mat_mapping.key(), renderer::ObjectInstance::FrontSide, material->get_name());
                    //front_material_mappings.insert(entry.second, material_info.m_name);
                }
            }
        }
    }

  private:
    MaterialMap             m_material_map;
    renderer::Project&      m_project;
    const InstanceMap&      m_instances;
};

class InteractiveRendererController
  : public renderer::DefaultRendererController
{
  public:
    InteractiveRendererController();

    void on_rendering_begin() override;
    Status get_status() const override;

    void set_status(const Status status);

    void schedule_update(std::unique_ptr<ScheduledAction> updater);

  private:
    std::vector<std::unique_ptr<ScheduledAction>>   m_scheduled_actions;
    Status                                          m_status;
};
