
//
// This source file is part of appleseed.
// Visit https://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2015-2018 Francois Beaune, The appleseedhq Organization
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

// Build options header.
#include "foundation/core/buildoptions.h"

// appleseed-max headers.
#include "appleseedrenderer/renderersettings.h"

// appleseed.foundation headers.
#include "foundation/memory/autoreleaseptr.h"

// 3ds Max headers.
#include "appleseed-max-common/_beginmaxheaders.h"
#include <maxtypes.h>
#include <render.h>
#include "appleseed-max-common/_endmaxheaders.h"

// Standard headers.
#include <map>
#include <vector>

// Forward declarations.
namespace renderer { class Assembly; }
namespace renderer { class AssemblyInstance; }
namespace renderer { class Camera; }
namespace renderer { class ObjectInstance; }
namespace renderer { class Project; }
class Bitmap;
class FrameRendParams;
class IAppleseedGeometricObject;
class IAppleseedMtl;
class MaxSceneEntities;
class RendParams;
class ViewParams;

enum class RenderType
{
    Default,
    MaterialPreview
};

struct ObjectInfo
{
    IAppleseedGeometricObject*          m_appleseed_geo_object = {};    // appleseed-max geometric object interface implemented by the 3ds Max object
    std::string                         m_name;                         // name of the appleseed object
    std::map<MtlID, std::string>        m_mtlid_to_slot_name;           // map a Max's material ID to an appleseed's material slot name
    std::map<MtlID, std::uint32_t>      m_mtlid_to_slot_index;          // map a Max's material ID to an appleseed's material slot index
};

typedef std::map<Object*, std::vector<ObjectInfo>> ObjectMap;
typedef std::map<std::string, renderer::ObjectInstance*> ObjectInstanceMap;
typedef std::map<std::string, renderer::AssemblyInstance*> AssemblyInstanceMap;
typedef std::map<Mtl*, std::string> MaterialMap;
typedef std::map<IAppleseedMtl*, std::string> IAppleseedMtlMap;
typedef std::map<Object*, std::string> AssemblyMap;

// Build an appleseed project from the current 3ds Max scene.
foundation::auto_release_ptr<renderer::Project> build_project(
    const MaxSceneEntities&             entities,
    const std::vector<DefaultLight>&    default_lights,
    INode*                              view_node,
    const ViewParams&                   view_params,
    const RendParams&                   rend_params,
    const FrameRendParams&              frame_rend_params,
    const RendererSettings&             settings,
    Bitmap*                             bitmap,
    const TimeValue                     time,
    RendProgressCallback*               progress_cb,
    ObjectMap&                          object_map,
    ObjectInstanceMap&                  object_inst_map,
    MaterialMap&                        material_map,
    AssemblyMap&                        assembly_map,
    AssemblyInstanceMap&                assembly_inst_map);

foundation::auto_release_ptr<renderer::Camera> build_camera(
    INode*                              view_node,
    const ViewParams&                   view_params,
    Bitmap*                             bitmap,
    const RendererSettings&             settings,
    const TimeValue                     time);

void add_object(
    renderer::Project&                  project,
    renderer::Assembly&                 assembly,
    INode*                              node,
    const RenderType                    type,
    const RendererSettings&             settings,
    const TimeValue                     time,
    ObjectMap&                          object_map,
    ObjectInstanceMap&                  instance_map,
    MaterialMap&                        material_map,
    AssemblyMap&                        assembly_map,
    AssemblyInstanceMap&                assembly_inst_map);
