
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
#include "interactiverenderercontroller.h"

// appleseed-max headers.
#include "appleseedinteractive/interactivesession.h"

// 3ds Max headers.
#include "appleseed-max-common/_beginmaxheaders.h"
#include <interactiverender.h>
#include "appleseed-max-common/_endmaxheaders.h"

// Standard headers.
#include <utility>

namespace asr = renderer;


void UpdateObjectInstanceAction::update()
{
    // todo: handle objects with optimized_for_instancing flag on
    renderer::Assembly* assembly = m_session->m_project->get_scene()->assemblies().get_by_name("assembly");

    if (m_session->m_object_inst_map.count(wide_to_utf8(m_node->GetName())) > 0)
    {
        m_session->m_object_map.erase(m_node->GetObjectRef());
        asr::ObjectInstance* object_instance = static_cast<asr::ObjectInstance*>(m_session->m_object_inst_map[wide_to_utf8(m_node->GetName())]);
        assembly->object_instances().remove(object_instance);

        add_object(
            *m_session->m_project,
            *assembly,
            m_node,
            RenderType::Default,
            m_session->m_renderer_settings,
            GetCOREInterface()->GetTime(),
            m_session->m_object_map,
            m_session->m_object_inst_map,
            m_session->m_material_map,
            m_session->m_assembly_map,
            m_session->m_assembly_inst_map);

       
        assembly->bump_version_id();

        return;
    }

    if (m_session->m_assembly_inst_map.count(wide_to_utf8(m_node->GetName())) > 0)
    {
       m_session->m_assembly_map.erase(m_node->GetObjectRef());
       asr::AssemblyInstance* assembly_instance = static_cast<asr::AssemblyInstance*>(m_session->m_assembly_inst_map[wide_to_utf8(m_node->GetName())]);
       assembly->assembly_instances().remove(assembly_instance);

       add_object(
           *m_session->m_project,
           *assembly,
           m_node,
           RenderType::Default,
           m_session->m_renderer_settings,
           GetCOREInterface()->GetTime(),
           m_session->m_object_map,
           m_session->m_object_inst_map,
           m_session->m_material_map,
           m_session->m_assembly_map,
           m_session->m_assembly_inst_map);

       assembly->bump_version_id();

       return;
    }
}


void RemoveObjectInstanceAction::update()
{
    renderer::Assembly* assembly = m_session->m_project->get_scene()->assemblies().get_by_name("assembly");

    if (m_session->m_object_inst_map.count(wide_to_utf8(m_node->GetName())) > 0)
    {
        m_session->m_object_map.erase(m_node->GetObjectRef());
        asr::ObjectInstance* object_instance = static_cast<asr::ObjectInstance*>(m_session->m_object_inst_map[wide_to_utf8(m_node->GetName())]);
        assembly->object_instances().remove(object_instance);
       
        assembly->bump_version_id();

        return;
    }

    if (m_session->m_assembly_inst_map.count(wide_to_utf8(m_node->GetName())) > 0)
    {
       m_session->m_assembly_map.erase(m_node->GetObjectRef());
       asr::AssemblyInstance* assembly_instance = static_cast<asr::AssemblyInstance*>(m_session->m_assembly_inst_map[wide_to_utf8(m_node->GetName())]);
       assembly->assembly_instances().remove(assembly_instance);

       assembly->bump_version_id();

       return;
    }
}

void AddObjectInstanceAction::update()
{
    renderer::Assembly* assembly = m_session->m_project->get_scene()->assemblies().get_by_name("assembly");

    add_object(
        *m_session->m_project,
        *assembly,
        m_node,
        RenderType::Default,
        m_session->m_renderer_settings,
        GetCOREInterface()->GetTime(),
        m_session->m_object_map,
        m_session->m_object_inst_map,
        m_session->m_material_map,
        m_session->m_assembly_map,
        m_session->m_assembly_inst_map);

    assembly->bump_version_id();
}

InteractiveRendererController::InteractiveRendererController()
  : m_status(ContinueRendering)
{
}

void InteractiveRendererController::on_rendering_begin()
{
    for (auto& updater : m_scheduled_actions)
        updater->update();
    
    m_scheduled_actions.clear();
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

void InteractiveRendererController::schedule_update(std::unique_ptr<ScheduledAction> updater)
{
    m_scheduled_actions.push_back(std::move(updater));
}
