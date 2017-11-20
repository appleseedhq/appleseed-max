
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2015-2017 Francois Beaune, The appleseedhq Organization
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
#include "projectbuilder.h"

// appleseed-max headers.
#include "appleseedenvmap/appleseedenvmap.h"
#include "appleseedobjpropsmod/appleseedobjpropsmod.h"
#include "appleseedrenderer/maxsceneentities.h"
#include "appleseedrenderer/renderersettings.h"
#include "iappleseedmtl.h"
#include "seexprutils.h"
#include "utilities.h"

// appleseed.renderer headers.
#include "renderer/api/camera.h"
#include "renderer/api/color.h"
#include "renderer/api/environment.h"
#include "renderer/api/environmentedf.h"
#include "renderer/api/environmentshader.h"
#include "renderer/api/frame.h"
#include "renderer/api/light.h"
#include "renderer/api/material.h"
#include "renderer/api/object.h"
#include "renderer/api/project.h"
#include "renderer/api/scene.h"
#include "renderer/api/texture.h"
#include "renderer/api/utility.h"

// appleseed.foundation headers.
#include "foundation/image/colorspace.h"
#include "foundation/image/genericimagefilewriter.h"
#include "foundation/image/image.h"
#include "foundation/math/aabb.h"
#include "foundation/math/scalar.h"
#include "foundation/math/transform.h"
#include "foundation/math/vector.h"
#include "foundation/platform/types.h"
#include "foundation/utility/containers/dictionary.h"
#include "foundation/utility/iostreamop.h"
#include "foundation/utility/searchpaths.h"

// 3ds Max headers.
#include <assert1.h>
#include <bitmap.h>
#include <genlight.h>
#include <iInstanceMgr.h>
#include <INodeTab.h>
#include <modstack.h>
#include <object.h>
#include <RendType.h>
#if MAX_RELEASE >= 18000
#include <Scene/IPhysicalCamera.h>
#endif
#include <trig.h>
#include <triobj.h>

// Standard headers.
#include <cstddef>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace asf = foundation;
namespace asr = renderer;

namespace
{
    std::string insert_color(
        asr::BaseGroup&         base_group,
        std::string             name,
        const asf::Color3f&     linear_rgb)
    {
        name = make_unique_name(base_group.colors(), name);

        base_group.colors().insert(
            asr::ColorEntityFactory::create(
                name.c_str(),
                asr::ParamArray()
                    .insert("color_space", "linear_rgb")
                    .insert("color", linear_rgb)));

        return name;
    }

    std::string insert_empty_material(
        asr::Assembly&          assembly,
        std::string             name)
    {
        name = make_unique_name(assembly.materials(), name);

        assembly.materials().insert(
            asr::GenericMaterialFactory().create(
                name.c_str(),
                asr::ParamArray()));

        return name;
    }

    std::string insert_default_material(
        asr::Assembly&          assembly,
        std::string             name,
        const asf::Color3f&     linear_rgb)
    {
        name = make_unique_name(assembly.materials(), name);

        asf::auto_release_ptr<asr::Material> material(
            asr::DisneyMaterialFactory().create(
                name.c_str(),
                asr::ParamArray()));

        // The Disney material expects sRGB colors, so we have to convert the input color to sRGB.
        static_cast<asr::DisneyMaterial*>(material.get())->add_layer(
            asr::DisneyMaterialLayer::get_default_values()
                .insert("base_color", fmt_se_expr(asf::linear_rgb_to_srgb(linear_rgb)))
                .insert("specular", 1.0)
                .insert("roughness", 0.625));

        assembly.materials().insert(material);

        return name;
    }

    class NullView
      : public View
    {
      public:
        NullView()
        {
            worldToView.IdentityMatrix();
            screenW = 640.0f;
            screenH = 480.0f;
        }

        virtual Point2 ViewToScreen(Point3 p) override
        {
            return Point2(p.x, p.y);
        }
    };

    struct ObjectInfo
    {
        std::string                     m_name;             // name of the appleseed object
        std::map<MtlID, asf::uint32>    m_mtlid_to_slot;    // map a 3ds Max's material ID to an appleseed's material slot
    };

    asf::auto_release_ptr<asr::MeshObject> convert_mesh_object(
        Mesh&                   mesh,
        const Matrix3&          mesh_transform,
        ObjectInfo&             object_info)
    {
        asf::auto_release_ptr<asr::MeshObject> object(
            asr::MeshObjectFactory().create(object_info.m_name.c_str(), asr::ParamArray()));

        // Make sure the input mesh has vertex normals.
        mesh.checkNormals(TRUE);

        // Copy vertices to the mesh object.
        object->reserve_vertices(mesh.getNumVerts());
        for (int i = 0, e = mesh.getNumVerts(); i < e; ++i)
        {
            const Point3 v = mesh_transform * mesh.getVert(i);
            object->push_vertex(asr::GVector3(v.x, v.y, v.z));
        }

        // Copy texture vertices to the mesh object.
        object->reserve_tex_coords(mesh.getNumTVerts());
        for (int i = 0, e = mesh.getNumTVerts(); i < e; ++i)
        {
            const UVVert& uv = mesh.getTVert(i);
            object->push_tex_coords(asr::GVector2(uv.x, uv.y));
        }

        // Compute the matrix to transform normals.
        Matrix3 normal_transform = mesh_transform;
        normal_transform.Invert();
        normal_transform = transpose(normal_transform);

        // Copy vertex normals and triangles to mesh object.
        object->reserve_vertex_normals(mesh.getNumFaces() * 3);
        object->reserve_triangles(mesh.getNumFaces());
        for (int i = 0, e = mesh.getNumFaces(); i < e; ++i)
        {
            Face& face = mesh.faces[i];
            TVFace& tvface = mesh.tvFace[i];

            const DWORD face_smgroup = face.getSmGroup();
            const MtlID face_mat = face.getMatID();

            asf::uint32 normal_indices[3];
            if (face_smgroup == 0)
            {
                // No smooth group for this face, use the face normal.
                const Point3 n = normal_transform * mesh.getFaceNormal(i);
                const asf::uint32 normal_index =
                    static_cast<asf::uint32>(
                        object->push_vertex_normal(
                            asf::safe_normalize(asr::GVector3(n.x, n.y, n.z))));
                normal_indices[0] = normal_index;
                normal_indices[1] = normal_index;
                normal_indices[2] = normal_index;
            }
            else
            {
                for (int j = 0; j < 3; ++j)
                {
                    RVertex& rvertex = mesh.getRVert(face.getVert(j));
                    const size_t normal_count = rvertex.rFlags & NORCT_MASK;
                    if (normal_count == 1)
                    {
                        // This vertex has a single normal.
                        const Point3& n = rvertex.rn.getNormal();
                        normal_indices[j] =
                            static_cast<asf::uint32>(
                                object->push_vertex_normal(
                                    asf::safe_normalize(asr::GVector3(n.x, n.y, n.z))));
                    }
                    else
                    {
                        // This vertex has multiple normals.
                        for (size_t k = 0; k < normal_count; ++k)
                        {
                            // Find the normal for this smooth group and material.
                            RNormal& rn = rvertex.ern[k];
                            if ((face_smgroup & rn.getSmGroup()) && face_mat == rn.getMtlIndex())
                            {
                                const Point3& n = rn.getNormal();
                                normal_indices[j] =
                                    static_cast<asf::uint32>(
                                        object->push_vertex_normal(
                                            asf::safe_normalize(asr::GVector3(n.x, n.y, n.z))));
                                break;
                            }
                        }
                    }
                }
            }

            static_assert(
                sizeof(DWORD) == sizeof(asf::uint32),
                "DWORD is expected to be 32-bit long");

            asr::Triangle triangle;
            triangle.m_v0 = face.getVert(0);
            triangle.m_v1 = face.getVert(1);
            triangle.m_v2 = face.getVert(2);
            triangle.m_n0 = normal_indices[0];
            triangle.m_n1 = normal_indices[1];
            triangle.m_n2 = normal_indices[2];
            if (mesh.getNumTVerts() > 0)
            {
                triangle.m_a0 = tvface.getTVert(0);
                triangle.m_a1 = tvface.getTVert(1);
                triangle.m_a2 = tvface.getTVert(2);
            }
            else
            {
                triangle.m_a0 = asr::Triangle::None;
                triangle.m_a1 = asr::Triangle::None;
                triangle.m_a2 = asr::Triangle::None;
            }

            // Assign to the triangle the material slot corresponding to the face's material ID,
            // creating a new material slot if necessary.
            asf::uint32 slot;
            const MtlID mtlid = face.getMatID();
            const auto it = object_info.m_mtlid_to_slot.find(mtlid);
            if (it == object_info.m_mtlid_to_slot.end())
            {
                // Create a new material slot in the object.
                const auto slot_name = "material_slot_" + asf::to_string(object->get_material_slot_count());
                slot = static_cast<asf::uint32>(object->push_material_slot(slot_name.c_str()));
                object_info.m_mtlid_to_slot.insert(std::make_pair(mtlid, slot));
            }
            else slot = it->second;
            triangle.m_pa = slot;

            object->push_triangle(triangle);
        }

        // todo: optimize the object.

        return object;
    }

    std::vector<ObjectInfo> create_mesh_objects(
        asr::Assembly&          assembly,
        INode*                  object_node,
        const TimeValue         time)
    {
        std::vector<ObjectInfo> object_infos;

        // Retrieve the GeomObject at the desired time.
        const ObjectState object_state = object_node->EvalWorldState(time);
        GeomObject* geom_object = static_cast<GeomObject*>(object_state.obj);

        // Create one appleseed MeshObject per 3ds Max Mesh.
        const int render_mesh_count = geom_object->NumberOfRenderMeshes();
        if (render_mesh_count > 0)
        {
            for (int i = 0; i < render_mesh_count; ++i)
            {
                NullView view;
                BOOL need_delete;
                Mesh* mesh = geom_object->GetMultipleRenderMesh(time, object_node, view, need_delete, i);
                if (mesh != nullptr)
                {
                    ObjectInfo object_info;
                    object_info.m_name = wide_to_utf8(object_node->GetName());
                    object_info.m_name = make_unique_name(assembly.objects(), object_info.m_name);

                    Matrix3 mesh_transform;
                    Interval mesh_transform_validity;
                    geom_object->GetMultipleRenderMeshTM(time, object_node, view, i, mesh_transform, mesh_transform_validity);

                    assembly.objects().insert(
                        asf::auto_release_ptr<asr::Object>(
                            convert_mesh_object(*mesh, mesh_transform, object_info)));
            
                    if (need_delete)
                        mesh->DeleteThis();

                    object_infos.push_back(object_info);
                }
            }
        }
        else
        {
            NullView view;
            BOOL need_delete;
            Mesh* mesh = geom_object->GetRenderMesh(time, object_node, view, need_delete);
            if (mesh != nullptr)
            {
                ObjectInfo object_info;
                object_info.m_name = wide_to_utf8(object_node->GetName());
                object_info.m_name = make_unique_name(assembly.objects(), object_info.m_name);

                assembly.objects().insert(
                    asf::auto_release_ptr<asr::Object>(
                        convert_mesh_object(*mesh, Matrix3(TRUE), object_info)));

                if (need_delete)
                    mesh->DeleteThis();

                object_infos.push_back(object_info);
            }
        }

        return object_infos;
    }

    typedef std::map<Mtl*, std::string> MaterialMap;

    struct MaterialInfo
    {
        std::string m_name;     // name of the appleseed material
        int         m_sides;    // sides of the object to which the material must be applied
    };

    MaterialInfo get_or_create_material(
        asr::Assembly&          assembly,
        const std::string&      instance_name,
        Mtl*                    mtl,
        MaterialMap&            material_map,
        const bool              use_max_procedural_maps)
    {
        MaterialInfo material_info;

        auto appleseed_mtl =
            static_cast<IAppleseedMtl*>(mtl->GetInterface(IAppleseedMtl::interface_id()));
        if (appleseed_mtl)
        {
            // It's an appleseed material.
            const auto it = material_map.find(mtl);
            if (it == material_map.end())
            {
                // The appleseed material does not exist yet, let the material plugin create it.
                material_info.m_name =
                    make_unique_name(assembly.materials(), wide_to_utf8(mtl->GetName()) + "_mat");
                assembly.materials().insert(
                    appleseed_mtl->create_material(assembly, material_info.m_name.c_str(), use_max_procedural_maps));
                material_map.insert(std::make_pair(mtl, material_info.m_name));
            }
            else
            {
                // The appleseed material already exists.
                material_info.m_name = it->second;
            }
            material_info.m_sides = appleseed_mtl->get_sides();
        }
        else
        {
            // It isn't an appleseed material: return an empty material that will appear black.
            material_info.m_name = insert_empty_material(assembly, instance_name + "_mat");
            material_info.m_sides = asr::ObjectInstance::FrontSide | asr::ObjectInstance::BackSide;
        }

        return material_info;
    }

    asr::VisibilityFlags::Type get_visibility_flags(Object* object, const TimeValue time)
    {
        if (object->SuperClassID() == GEN_DERIVOB_CLASS_ID)
        {
            IDerivedObject* derived_object = static_cast<IDerivedObject*>(object);
            for (int i = 0, e = derived_object->NumModifiers(); i < e; ++i)
            {
                Modifier* modifier = derived_object->GetModifier(i);
                if (modifier->ClassID() == AppleseedObjPropsMod::get_class_id())
                {
                    const auto obj_props_mod = static_cast<const AppleseedObjPropsMod*>(modifier);
                    return obj_props_mod->get_visibility_flags(time);
                }
            }
        }

        return asr::VisibilityFlags::AllRays;
    }

    enum class RenderType
    {
        Default,
        MaterialPreview
    };

    void create_object_instance(
        asr::Assembly&          assembly,
        INode*                  instance_node,
        const ObjectInfo&       object_info,
        const RenderType        type,
        const bool              use_max_proc_maps,
        const TimeValue         time,
        MaterialMap&            material_map)
    {
        // Compute a unique name for this instance.
        const std::string instance_name =
            make_unique_name(assembly.object_instances(), object_info.m_name + "_inst");

        // Material mappings.
        asf::StringDictionary front_material_mappings;
        asf::StringDictionary back_material_mappings;

        // Retrieve or create an appleseed material.
        Mtl* mtl = instance_node->GetMtl();
        if (mtl)
        {
            // The instance has a material.

            // Trigger SME materials update.
            if (type == RenderType::MaterialPreview)
                mtl->Update(time, FOREVER);

            const int submtlcount = mtl->NumSubMtls();
            if (mtl->IsMultiMtl() && submtlcount > 0)
            {
                // It's a multi/sub-object material.
                for (int i = 0; i < submtlcount; ++i)
                {
                    Mtl* submtl = mtl->GetSubMtl(i);
                    if (submtl != nullptr)
                    {
                        const auto material_info =
                            get_or_create_material(
                                assembly,
                                instance_name,
                                submtl,
                                material_map,
                                use_max_proc_maps);

                        const auto entry = object_info.m_mtlid_to_slot.find(i);
                        if (entry != object_info.m_mtlid_to_slot.end())
                        {
                            const std::string slot_name = "material_slot_" + asf::to_string(entry->second);

                            if (material_info.m_sides & asr::ObjectInstance::FrontSide)
                                front_material_mappings.insert(slot_name, material_info.m_name);

                            if (material_info.m_sides & asr::ObjectInstance::BackSide)
                                back_material_mappings.insert(slot_name, material_info.m_name);
                        }
                    }
                }
            }
            else
            {
                // It's a single material.

                // Create the appleseed material.
                const auto material_info =
                    get_or_create_material(
                        assembly,
                        instance_name,
                        mtl,
                        material_map,
                        use_max_proc_maps);

                // Assign it to all material slots.
                for (const auto& entry : object_info.m_mtlid_to_slot)
                {
                    const std::string slot_name = "material_slot_" + asf::to_string(entry.second);

                    if (material_info.m_sides & asr::ObjectInstance::FrontSide)
                        front_material_mappings.insert(slot_name, material_info.m_name);

                    if (material_info.m_sides & asr::ObjectInstance::BackSide)
                        back_material_mappings.insert(slot_name, material_info.m_name);
                }
            }
        }
        else
        {
            // The instance does not have a material.

            // Create a new default material.
            const std::string material_name =
                insert_default_material(
                    assembly,
                    instance_name + "_mat",
                    to_color3f(Color(instance_node->GetWireColor())));

            // Assign it to all material slots.
            for (const auto& entry : object_info.m_mtlid_to_slot)
            {
                const std::string slot_name = "material_slot_" + asf::to_string(entry.second);
                front_material_mappings.insert(slot_name, material_name);
                back_material_mappings.insert(slot_name, material_name);
            }
        }

        // Parameters.
        asr::ParamArray params;
        params.insert(
            "visibility",
            asr::VisibilityFlags::to_dictionary(
                get_visibility_flags(instance_node->GetObjectRef(), time)));
        if (type == RenderType::MaterialPreview)
            params.insert_path("visibility.shadow", false);

        // Compute the transform of this instance.
        const asf::Transformd transform =
            asf::Transformd::from_local_to_parent(
                to_matrix4d(instance_node->GetObjTMAfterWSM(time)));

        // Create the instance and insert it into the assembly.
        assembly.object_instances().insert(
            asr::ObjectInstanceFactory::create(
                instance_name.c_str(),
                params,
                object_info.m_name.c_str(),
                transform,
                front_material_mappings,
                back_material_mappings));
    }

    typedef std::map<Object*, std::vector<ObjectInfo>> ObjectMap;

    void add_object(
        asr::Assembly&          assembly,
        INode*                  node,
        const RenderType        type,
        const bool              use_max_proc_maps,
        const TimeValue         time,
        ObjectMap&              object_map,
        MaterialMap&            material_map)
    {
        // Retrieve the geometrical object referenced by this node.
        Object* object = node->GetObjectRef();

        // Check if we already generated the corresponding appleseed object.
        const ObjectMap::const_iterator it = object_map.find(object);

        if (it == object_map.end())
        {
            // The appleseed objects do not exist yet, create and instantiate them.
            const auto object_infos = create_mesh_objects(assembly, node, time);
            object_map.insert(std::make_pair(object, object_infos));
            for (const auto& object_info : object_infos)
            {
                create_object_instance(
                    assembly,
                    node,
                    object_info,
                    type,
                    use_max_proc_maps,
                    time,
                    material_map);
            }
        }
        else
        {
            // The appleseed objects already exist, simply instantiate them.
            for (const auto& object_info : it->second)
            {
                create_object_instance(
                    assembly,
                    node,
                    object_info,
                    type,
                    use_max_proc_maps,
                    time,
                    material_map);
            }
        }
    }

    void add_objects(
        asr::Assembly&          assembly,
        const MaxSceneEntities& entities,
        const RenderType        type,
        const bool              use_max_proc_maps,
        const TimeValue         time,
        ObjectMap&              object_map,
        MaterialMap&            material_map)
    {
        for (const auto& object : entities.m_objects)
        {
            add_object(
                assembly,
                object,
                type,
                use_max_proc_maps,
                time,
                object_map,
                material_map);
        }
    }

    void add_omni_light(
        asr::Assembly&          assembly,
        const std::string&      light_name,
        const asf::Transformd&  transform,
        const std::string&      color_name,
        const float             intensity,
        const float             decay_start,
        const int               decay_exponent)
    {
        asf::auto_release_ptr<asr::Light> light(
            asr::MaxOmniLightFactory().create(
                light_name.c_str(),
                asr::ParamArray()
                    .insert("intensity", color_name)
                    .insert("intensity_multiplier", intensity * asf::Pi<float>())
                    .insert("decay_start", decay_start)
                    .insert("decay_exponent", decay_exponent)));
        light->set_transform(transform);
        assembly.lights().insert(light);
    }

    void add_spot_light(
        asr::Assembly&          assembly,
        const std::string&      light_name,
        const asf::Transformd&  transform,
        const std::string&      color_name,
        const float             intensity,
        const float             inner_angle,
        const float             outer_angle,
        const float             decay_start,
        const int               decay_exponent)
    {
        asf::auto_release_ptr<asr::Light> light(
            asr::MaxSpotLightFactory().create(
                light_name.c_str(),
                asr::ParamArray()
                    .insert("intensity", color_name)
                    .insert("intensity_multiplier", intensity * asf::Pi<float>())
                    .insert("inner_angle", inner_angle)
                    .insert("outer_angle", outer_angle)
                    .insert("decay_start", decay_start)
                    .insert("decay_exponent", decay_exponent)));
        light->set_transform(transform);
        assembly.lights().insert(light);
    }

    void add_directional_light(
        asr::Assembly&          assembly,
        const std::string&      light_name,
        const asf::Transformd&  transform,
        const std::string&      color_name,
        const float             intensity)
    {
        asf::auto_release_ptr<asr::Light> light(
            asr::DirectionalLightFactory().create(
                light_name.c_str(),
                asr::ParamArray()
                    .insert("irradiance", color_name)
                    .insert("irradiance_multiplier", intensity * asf::Pi<float>())));
        light->set_transform(transform);
        assembly.lights().insert(light);
    }

    void add_sun_light(
        asr::Assembly&          assembly,
        const std::string&      light_name,
        const asf::Transformd&  transform,
        const std::string&      color_name,
        const float             intensity,
        const float             size_mult,
        const char*             sky_name)
    {
        asf::auto_release_ptr<asr::Light> light(
            asr::SunLightFactory().create(
                light_name.c_str(),
                asr::ParamArray()
                  .insert("radiance_multiplier", intensity * asf::Pi<float>())
                  .insert("turbidity", 1.0)
                  .insert("environment_edf", sky_name)
                  .insert("size_multiplier", size_mult)));
        light->set_transform(transform);
        assembly.lights().insert(light);
    }

    void add_light(
        asr::Assembly&          assembly,
        const RendParams&       rend_params,
        INode*                  light_node,
        const TimeValue         time)
    {
        // Retrieve the ObjectState at the desired time.
        const ObjectState object_state = light_node->EvalWorldState(time);

        // Compute a unique name for this light.
        std::string light_name = wide_to_utf8(light_node->GetName());
        light_name = make_unique_name(assembly.lights(), light_name);

        // Compute the transform of this light.
        const asf::Transformd transform =
            asf::Transformd::from_local_to_parent(
                to_matrix4d(light_node->GetObjTMAfterWSM(time)));

        // Retrieve the light's parameters.
        GenLight* light_object = dynamic_cast<GenLight*>(object_state.obj);
        const asf::Color3f color = to_color3f(light_object->GetRGBColor(time));
        const float intensity = light_object->GetIntensity(time);
        const float decay_start = light_object->GetDecayRadius(time);
        const int decay_exponent = light_object->GetDecayType();

        // Create a color entity.
        const std::string color_name =
            insert_color(assembly, light_name + "_color", color);

        // Get light from envmap.
        INode* sun_node(nullptr);
        BOOL sun_node_on(FALSE);
        float sun_size_mult;
        if (rend_params.envMap != nullptr &&
            rend_params.envMap->IsSubClassOf(AppleseedEnvMap::get_class_id()))
        {
            AppleseedEnvMap* env_map = static_cast<AppleseedEnvMap*>(rend_params.envMap);
            get_paramblock_value_by_name(env_map->GetParamBlock(0), L"sun_node", time, sun_node, FOREVER);
            get_paramblock_value_by_name(env_map->GetParamBlock(0), L"sun_node", time, sun_node, FOREVER);
            get_paramblock_value_by_name(env_map->GetParamBlock(0), L"sun_node_on", time, sun_node_on, FOREVER);
            get_paramblock_value_by_name(env_map->GetParamBlock(0), L"sun_size_multiplier", time, sun_size_mult, FOREVER);
        }

        if (sun_node && sun_node_on && light_node == sun_node)
        {
            add_sun_light(
                assembly,
                light_name,
                transform,
                color_name,
                intensity,
                sun_size_mult,
                "environment_edf");
        }        
        else if (light_object->ClassID() == Class_ID(OMNI_LIGHT_CLASS_ID, 0))
        {
            add_omni_light(
                assembly,
                light_name,
                transform,
                color_name,
                intensity,
                decay_start,
                decay_exponent);
        }
        else if (light_object->ClassID() == Class_ID(SPOT_LIGHT_CLASS_ID, 0) ||
                 light_object->ClassID() == Class_ID(FSPOT_LIGHT_CLASS_ID, 0))
        {
            add_spot_light(
                assembly,
                light_name,
                transform,
                color_name,
                intensity,
                light_object->GetHotspot(time),
                light_object->GetFallsize(time),
                decay_start,
                decay_exponent);
        }
        else if (light_object->ClassID() == Class_ID(DIR_LIGHT_CLASS_ID, 0) ||
                 light_object->ClassID() == Class_ID(TDIR_LIGHT_CLASS_ID, 0))
        {
            add_directional_light(
                assembly,
                light_name,
                transform,
                color_name,
                intensity);
        }
        else
        {
            // Unsupported light type.
            // todo: emit warning message.
        }
    }

    void add_lights(
        asr::Assembly&          assembly,
        const RendParams&       rend_params,
        const MaxSceneEntities& entities,
        const TimeValue         time)
    {
        for (const auto& light_info : entities.m_lights)
        {
            if (light_info.m_enabled)
                add_light(assembly, rend_params, light_info.m_light, time);
        }
    }

    bool is_zero(const Matrix3& m)
    {
        for (int row = 0; row < 4; ++row)
        {
            for (int col = 0; col < 3; ++col)
            {
                if (m[row][col] != 0.0f)
                    return false;
            }
        }

        return true;
    }

    void add_default_lights(
        asr::Assembly&                      assembly,
        const std::vector<DefaultLight>&    default_lights)
    {
        for (const auto& light : default_lights)
        {
            // Compute the transform of this light.
            const asf::Transformd transform =
                is_zero(light.tm)
                    ? asf::Transformd::identity()   // todo: implement
                    : asf::Transformd::from_local_to_parent(to_matrix4d(light.tm));

            // Compute a unique name for this light.
            const std::string light_name =
                make_unique_name(assembly.lights(), "DefaultLight");

            // Create a color entity.
            const std::string color_name =
                insert_color(assembly, light_name + "_color", to_color3f(light.ls.color));

            // Add the light.
            switch (light.ls.type)
            {
              case OMNI_LGT:
                add_omni_light(
                    assembly,
                    light_name,
                    transform,
                    color_name,
                    light.ls.intens,
                    0.0f,   // decay start
                    0);     // decay exponent
                break;

              case SPOT_LGT:
                add_spot_light(
                    assembly,
                    light_name,
                    transform,
                    color_name,
                    light.ls.intens,
                    light.ls.hotsize,
                    light.ls.fallsize,
                    0.0f,   // decay start
                    0);     // decay exponent
                break;

              case DIRECT_LGT:
                add_directional_light(
                    assembly,
                    light_name,
                    transform,
                    color_name,
                    light.ls.intens);
                break;

              case AMBIENT_LGT:
                // todo: implement.
                break;
            }
        }
    }

    bool has_light_emitting_materials(const MaterialMap& material_map)
    {
        for (const auto& entry : material_map)
        {
            Mtl* mtl = entry.first;
            IAppleseedMtl* appleseed_mtl =
                static_cast<IAppleseedMtl*>(mtl->GetInterface(IAppleseedMtl::interface_id()));
            if (appleseed_mtl->can_emit_light())
                return true;
        }

        return false;
    }

    void populate_assembly(
        asr::Scene&                         scene,
        asr::Assembly&                      assembly,
        const RendParams&                   rend_params,
        const MaxSceneEntities&             entities,
        const std::vector<DefaultLight>&    default_lights,
        const RenderType                    type,
        const RendererSettings&             settings,
        const TimeValue                     time)
    {
        // Add objects, object instances and materials to the assembly.
        ObjectMap object_map;
        MaterialMap material_map;
        add_objects(
            assembly,
            entities,
            type,
            settings.m_use_max_procedural_maps,
            time,
            object_map,
            material_map);

        // Only add non-physical lights. Light-emitting materials were added by material plugins.
        add_lights(assembly, rend_params, entities, time);

        // Add Max's default lights if
        //       the scene does not contain non-physical lights (point lights, spot lights, etc.)
        //   and the scene does not contain light-emitting materials
        //   and the scene does not contain a light-emitting environment.
        const bool has_lights = !entities.m_lights.empty();
        const bool has_emitting_mats = has_light_emitting_materials(material_map);
        const bool has_emitting_env = !scene.get_environment()->get_parameters().get_optional<std::string>("environment_edf").empty();
        if (!has_lights &&
            !has_emitting_mats &&
            !(has_emitting_env && settings.m_background_emits_light))
            add_default_lights(assembly, default_lights);
    }

    void setup_solid_environment(
        asr::Scene&             scene,
        const FrameRendParams&  frame_rend_params,
        const RendererSettings& settings)
    {
        const asf::Color3f background_color =
            asf::clamp_low(
                to_color3f(frame_rend_params.background),
                0.0f);

        if (asf::is_zero(background_color))
        {
            scene.set_environment(
                asr::EnvironmentFactory::create(
                    "environment",
                    asr::ParamArray()));
        }
        else
        {
            const std::string background_color_name =
                insert_color(scene, "environment_edf_color", background_color);

            scene.environment_edfs().insert(
                asr::ConstantEnvironmentEDFFactory().create(
                    "environment_edf",
                    asr::ParamArray()
                        .insert("radiance", background_color_name)));

            scene.environment_shaders().insert(
                asr::EDFEnvironmentShaderFactory().create(
                    "environment_shader",
                    asr::ParamArray()
                        .insert("environment_edf", "environment_edf")
                        .insert("alpha_value", settings.m_background_alpha)));

            scene.set_environment(
                asr::EnvironmentFactory::create(
                    "environment",
                    asr::ParamArray()
                        .insert("environment_edf", "environment_edf")
                        .insert("environment_shader", "environment_shader")));
        }
    }

    void setup_environment_map(
        asr::Scene&             scene,
        const RendParams&       rend_params,
        const RendererSettings& settings,
        const TimeValue         time)
    {
        // Create environment EDF.
        if (rend_params.envMap->IsSubClassOf(AppleseedEnvMap::get_class_id()))
        {
            auto appleseed_envmap = static_cast<AppleseedEnvMap*>(rend_params.envMap);
            auto env_map = appleseed_envmap->create_envmap("environment_edf");
            scene.environment_edfs().insert(env_map);
        }
        else
        {
            std::string env_tex_instance_name;
            if (settings.m_use_max_procedural_maps)
            {
                env_tex_instance_name =
                    insert_procedural_texture_and_instance(
                        scene,
                        rend_params.envMap);
            }
            else if (is_bitmap_texture(rend_params.envMap))
            {
                env_tex_instance_name =
                    insert_bitmap_texture_and_instance(
                        scene,
                        static_cast<BitmapTex*>(rend_params.envMap));
            }
            else
            {
                // Dimensions of the environment map texture.
                const size_t EnvMapWidth = 2048;
                const size_t EnvMapHeight = 1024;

                // Render the environment map into a Max bitmap.
                BitmapInfo bi;
                bi.SetWidth(static_cast<WORD>(EnvMapWidth));
                bi.SetHeight(static_cast<WORD>(EnvMapHeight));
                bi.SetType(BMM_FLOAT_RGBA_32);
                Bitmap* envmap_bitmap = TheManager->Create(&bi);
                rend_params.envMap->RenderBitmap(time, envmap_bitmap, 1.0f, TRUE);

                // Render the Max bitmap to an appleseed image.
                asf::auto_release_ptr<asf::Image> envmap_image =
                    render_bitmap_to_image(envmap_bitmap, EnvMapWidth, EnvMapHeight, 32, 32);

                // Destroy the Max bitmap.
                envmap_bitmap->DeleteThis();

                // Write the environment map to disk, useful for debugging.
                // asf::GenericImageFileWriter writer;
                // writer.write("appleseed-max-environment-map.exr", envmap_image.ref());

                const std::string env_tex_name = make_unique_name(scene.textures(), "environment_map");
                scene.textures().insert(
                    asf::auto_release_ptr<asr::Texture>(
                        asr::MemoryTexture2dFactory().create(
                            env_tex_name.c_str(),
                            asr::ParamArray()
                                .insert("color_space", "linear_rgb"),
                            envmap_image)));

                env_tex_instance_name = make_unique_name(scene.texture_instances(), "environment_map_inst");
                scene.texture_instances().insert(
                    asf::auto_release_ptr<asr::TextureInstance>(
                        asr::TextureInstanceFactory::create(
                            env_tex_instance_name.c_str(),
                            asr::ParamArray(),
                            env_tex_name.c_str())));
            }

            asr::ParamArray env_edf_params;
            env_edf_params.insert("radiance", env_tex_instance_name.c_str());

            UVGen* uvgen = rend_params.envMap->GetTheUVGen();
            if (uvgen && uvgen->IsStdUVGen())
            {
                StdUVGen* std_uvgen = static_cast<StdUVGen*>(uvgen);
                env_edf_params.insert("horizontal_shift", std_uvgen->GetUOffs(time) * 180.0f);
                env_edf_params.insert("vertical_shift", std_uvgen->GetVOffs(time) * 180.0f);
            }

            scene.environment_edfs().insert(
                asf::auto_release_ptr<asr::EnvironmentEDF>(
                    asr::LatLongMapEnvironmentEDFFactory().create(
                        "environment_edf",
                        env_edf_params)));
        }

        // Create environment shader.
        scene.environment_shaders().insert(
            asr::EDFEnvironmentShaderFactory().create(
                "environment_shader",
                asr::ParamArray()
                    .insert("environment_edf", "environment_edf")
                    .insert("alpha_value", settings.m_background_alpha)));

        // Create environment.
        scene.set_environment(
            asr::EnvironmentFactory::create(
                "environment",
                asr::ParamArray()
                    .insert("environment_edf", "environment_edf")
                    .insert("environment_shader", "environment_shader")));
    }

    void setup_material_editor_environment_map(
        asr::Scene&             scene,
        const RendParams&       rend_params,
        const TimeValue         time)
    {
        // Dimensions of the environment map texture.
        const size_t EnvMapWidth = 512;
        const size_t EnvMapHeight = 512;

        // Render the environment map into a Max bitmap.
        BitmapInfo bi;
        bi.SetWidth(static_cast<WORD>(EnvMapWidth));
        bi.SetHeight(static_cast<WORD>(EnvMapHeight));
        bi.SetType(BMM_FLOAT_RGBA_32);
        Bitmap* envmap_bitmap = TheManager->Create(&bi);
        rend_params.envMap->RenderBitmap(time, envmap_bitmap, 1.0f, TRUE);

        // Render the Max bitmap to an appleseed image.
        asf::auto_release_ptr<asf::Image> envmap_image =
            render_bitmap_to_image(envmap_bitmap, EnvMapWidth, EnvMapHeight, 32, 32);

        // Destroy the Max bitmap.
        envmap_bitmap->DeleteThis();

        // Write the environment map to disk, useful for debugging.
        // asf::GenericImageFileWriter writer;
        // writer.write("appleseed-max-material-editor-environment-map.exr", envmap_image.ref());

        // Create texture entity from image.
        const std::string env_tex_name = make_unique_name(scene.textures(), "environment_map");
        scene.textures().insert(
            asf::auto_release_ptr<asr::Texture>(
                asr::MemoryTexture2dFactory().create(
                    env_tex_name.c_str(),
                    asr::ParamArray()
                        .insert("color_space", "linear_rgb"),
                    envmap_image)));

        // Create texture entity instance.
        const std::string env_tex_instance_name = make_unique_name(scene.texture_instances(), "environment_map_inst");
        scene.texture_instances().insert(
            asf::auto_release_ptr<asr::TextureInstance>(
                asr::TextureInstanceFactory::create(
                    env_tex_instance_name.c_str(),
                    asr::ParamArray(),
                    env_tex_name.c_str())));

        // Create environment EDF.
        asr::ParamArray env_edf_params;
        env_edf_params.insert("radiance", env_tex_instance_name.c_str());
        scene.environment_edfs().insert(
            asf::auto_release_ptr<asr::EnvironmentEDF>(
                asr::LatLongMapEnvironmentEDFFactory().create(
                    "environment_edf",
                    env_edf_params)));

        // Create environment shader.
        scene.environment_shaders().insert(
            asr::BackgroundEnvironmentShaderFactory().create(
                "environment_shader",
                asr::ParamArray()
                    .insert("color", env_tex_instance_name)
                    .insert("alpha", 1.0f)));

        // Create environment.
        scene.set_environment(
            asr::EnvironmentFactory::create(
                "environment",
                asr::ParamArray()
                    .insert("environment_edf", "environment_edf")
                    .insert("environment_shader", "environment_shader")));
    }

    void setup_environment(
        asr::Scene&             scene,
        const RendParams&       rend_params,
        const FrameRendParams&  frame_rend_params,
        const RendererSettings& settings,
        const TimeValue         time)
    {
        if (rend_params.envMap != nullptr)
        {
            if (rend_params.inMtlEdit)
                setup_material_editor_environment_map(scene, rend_params, time);
            else setup_environment_map(scene, rend_params, settings, time);
        }
        else setup_solid_environment(scene, frame_rend_params, settings);
    }

    asf::auto_release_ptr<asr::Frame> build_frame(
        const RendParams&       rend_params,
        const FrameRendParams&  frame_rend_params,
        Bitmap*                 bitmap,
        const RendererSettings& settings)
    {
        if (rend_params.inMtlEdit)
        {
            return
                asr::FrameFactory::create(
                    "beauty",
                    asr::ParamArray()
                        .insert("camera", "camera")
                        .insert("resolution", asf::Vector2i(bitmap->Width(), bitmap->Height()))
                        .insert("tile_size", asf::Vector2i(8, 8))
                        .insert("color_space", "linear_rgb")
                        .insert("filter", "box")
                        .insert("filter_size", 0.5));
        }
        else
        {
            asf::auto_release_ptr<asr::Frame> frame(
                asr::FrameFactory::create(
                    "beauty",
                    asr::ParamArray()
                        .insert("camera", "camera")
                        .insert("resolution", asf::Vector2i(bitmap->Width(), bitmap->Height()))
                        .insert("tile_size", asf::Vector2i(settings.m_tile_size))
                        .insert("color_space", "linear_rgb")
                        .insert("filter", "blackman-harris")
                        .insert("filter_size", 1.5)));

            if (rend_params.rendType == RENDTYPE_REGION)
            {
                frame->set_crop_window(
                    asf::AABB2u(
                        asf::Vector2u(frame_rend_params.regxmin, frame_rend_params.regymin),
                        asf::Vector2u(frame_rend_params.regxmax, frame_rend_params.regymax)));
            }

            return frame;
        }
    }
}

asf::auto_release_ptr<asr::Camera> build_camera(
    INode*                  view_node,
    const ViewParams&       view_params,
    Bitmap*                 bitmap,
    const RendererSettings& settings,
    const TimeValue         time)
{
    asf::auto_release_ptr<renderer::Camera> camera;

    if (view_params.projType == PROJ_PARALLEL)
    {
        //
        // Orthographic camera.
        //

        asr::ParamArray params;

        // Film dimensions.
        const float ViewDefaultWidth = 400.0f;
        const float aspect = static_cast<float>(bitmap->Height()) / bitmap->Width();
        const float film_width = ViewDefaultWidth * view_params.zoom;
        const float film_height = film_width * aspect;
        params.insert("film_dimensions", asf::Vector2f(film_width, film_height));

        // Create camera.
        camera = asr::OrthographicCameraFactory().create("camera", params);
    }
    else
    {
        //
        // Perspective camera.
        //

        DbgAssert(view_params.projType == PROJ_PERSPECTIVE);

        asr::ParamArray params;
        params.insert("horizontal_fov", asf::rad_to_deg(view_params.fov));

#if MAX_RELEASE >= 18000
        // Look for a physical camera in th scene.
        MaxSDK::IPhysicalCamera* phys_camera = nullptr;
        if (view_node)
            phys_camera = dynamic_cast<MaxSDK::IPhysicalCamera*>(view_node->EvalWorldState(time).obj);

        if (phys_camera && phys_camera->GetDOFEnabled(time, FOREVER))
        {
            // Film dimensions.
            const float aspect = static_cast<float>(bitmap->Height()) / bitmap->Width();
            const float film_width = phys_camera->GetFilmWidth(time, FOREVER);
            const float film_height = film_width * aspect;
            params.insert("film_dimensions", asf::Vector2f(film_width, film_height));

            // DOF settings.
            params.insert("f_stop", phys_camera->GetLensApertureFNumber(time, FOREVER));
            params.insert("focal_distance", phys_camera->GetFocusDistance(time, FOREVER));
            switch (phys_camera->GetBokehShape(time, FOREVER))
            {
              case MaxSDK::IPhysicalCamera::BokehShape::Circular:
                params.insert("diaphragm_blades", 0);
                break;
              case MaxSDK::IPhysicalCamera::BokehShape::Bladed:
                params.insert("diaphragm_blades", phys_camera->GetBokehNumberOfBlades(time, FOREVER));
                params.insert("diaphragm_tilt_angle", phys_camera->GetBokehBladesRotationDegrees(time, FOREVER));
                break;
            }

            // Create camera.
            camera = asr::ThinLensCameraFactory().create("camera", params);
        }
        else
#endif
        {
            // Film dimensions.
            // todo: using dummy values for now, can we and should we do better?
            params.insert("film_dimensions", asf::Vector2i(bitmap->Width(), bitmap->Height()));

            // Create camera.
            camera = asr::PinholeCameraFactory().create("camera", params);
        }
    }

    // Set camera transform.
    camera->transform_sequence().set_transform(
        0.0,
        asf::Transformd::from_local_to_parent(
            asf::Matrix4d::make_scaling(asf::Vector3d(settings.m_scale_multiplier)) *
            to_matrix4d(Inverse(view_params.affineTM))));

    return camera;
}

asf::auto_release_ptr<asr::Project> build_project(
    const MaxSceneEntities&                 entities,
    const std::vector<DefaultLight>&        default_lights,
    INode*                                  view_node,
    const ViewParams&                       view_params,
    const RendParams&                       rend_params,
    const FrameRendParams&                  frame_rend_params,
    const RendererSettings&                 settings,
    Bitmap*                                 bitmap,
    const TimeValue                         time)
{
    // Create an empty project.
    asf::auto_release_ptr<asr::Project> project(
        asr::ProjectFactory::create("project"));

    // Initialize search paths.
    project->search_paths().set_root_path(get_root_path());
    project->search_paths().push_back("shaders");

    // Add default configurations to the project.
    project->add_default_configurations();

    // Create a scene.
    asf::auto_release_ptr<asr::Scene> scene(asr::SceneFactory::create());

    // Setup the environment.
    setup_environment(
        scene.ref(),
        rend_params,
        frame_rend_params,
        settings,
        time);

    // Create an assembly.
    asf::auto_release_ptr<asr::Assembly> assembly(
        asr::AssemblyFactory().create("assembly"));

    // Populate the assembly with entities from the 3ds Max scene.
    const RenderType type =
        rend_params.inMtlEdit ? RenderType::MaterialPreview : RenderType::Default;
    populate_assembly(
        scene.ref(),
        assembly.ref(),
        rend_params,
        entities,
        default_lights,
        type,
        settings,
        time);

    // Create an instance of the assembly and insert it into the scene.
    asf::auto_release_ptr<asr::AssemblyInstance> assembly_instance(
        asr::AssemblyInstanceFactory::create(
            "assembly_inst",
            asr::ParamArray(),
            "assembly"));
    assembly_instance->transform_sequence()
        .set_transform(0.0, asf::Transformd::from_local_to_parent(
            asf::Matrix4d::make_scaling(asf::Vector3d(settings.m_scale_multiplier))));
    scene->assembly_instances().insert(assembly_instance);

    // Insert the assembly into the scene.
    scene->assemblies().insert(assembly);

    // Create a camera and bind it to the scene.
    scene->cameras().insert(
        build_camera(view_node, view_params, bitmap, settings, time));

    // Create a frame and bind it to the project.
    project->set_frame(
        build_frame(
            rend_params,
            frame_rend_params,
            bitmap,
            settings));

    // Bind the scene to the project.
    project->set_scene(scene);

    // Apply renderer settings.
    settings.apply(project.ref());

    return project;
}
