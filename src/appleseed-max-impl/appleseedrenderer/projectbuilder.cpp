
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

// Interface header.
#include "projectbuilder.h"

// appleseed-max headers.
#include "appleseedenvmap/appleseedenvmap.h"
#include "appleseedglassmtl/appleseedglassmtl.h"
#include "appleseedobjpropsmod/appleseedobjpropsmod.h"
#include "appleseedrenderelement/appleseedrenderelement.h"
#include "appleseedrenderer/maxsceneentities.h"
#include "appleseedrenderer/renderersettings.h"
#include "iappleseedgeometricobject.h"
#include "iappleseedmtl.h"
#include "seexprutils.h"
#include "utilities.h"

// Build options header.
#include "renderer/api/buildoptions.h"

// appleseed.renderer headers.
#include "renderer/api/aov.h"
#include "renderer/api/camera.h"
#include "renderer/api/color.h"
#include "renderer/api/edf.h"
#include "renderer/api/environment.h"
#include "renderer/api/environmentedf.h"
#include "renderer/api/environmentshader.h"
#include "renderer/api/frame.h"
#include "renderer/api/light.h"
#include "renderer/api/material.h"
#include "renderer/api/object.h"
#include "renderer/api/postprocessing.h"
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
#include "_beginmaxheaders.h"
#include <assert1.h>
#include <bitmap.h>
#include <genlight.h>
#include <iInstanceMgr.h>
#include <INodeTab.h>
#include <lslights.h>
#include <modstack.h>
#include <object.h>
#include <pbbitmap.h>
#include <renderelements.h>
#include <RendType.h>
#include <Scene/IPhysicalCamera.h>
#include <trig.h>
#include <triobj.h>
#include "_endmaxheaders.h"

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

        Point2 ViewToScreen(Point3 p) override
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
                        convert_mesh_object(*mesh, Matrix3(TRUE), object_info)));   // can't use Matrix3::Identity (link error)

                if (need_delete)
                    mesh->DeleteThis();

                object_infos.push_back(object_info);
            }
        }

        return object_infos;
    }

    std::vector<ObjectInfo> create_objects(
        asr::Assembly&          assembly,
        INode*                  object_node,
        const TimeValue         time)
    {
        // Retrieve the geometrical object referenced by this node.
        Object* object = object_node->GetObjectRef();

        // Check if this object is defined by an appleseed-max plugin.
        auto appleseed_geo_object =
            static_cast<IAppleseedGeometricObject*>(object->GetInterface(IAppleseedGeometricObject::interface_id()));

        if (appleseed_geo_object)
        {
            ObjectInfo object_info;
            object_info.m_name = wide_to_utf8(object_node->GetName());
            object_info.m_name = make_unique_name(assembly.objects(), object_info.m_name);

            // Create the object and insert it into the assembly.
            asf::auto_release_ptr<asr::Object> object =
                appleseed_geo_object->create_object(assembly, object_info.m_name.c_str());
            assembly.objects().insert(object);

            return { object_info };
        }
        else
        {
            return create_mesh_objects(assembly, object_node, time);
        }
    }

    typedef std::map<Mtl*, std::string> MaterialMap;

    struct MaterialInfo
    {
        std::string m_name;     // name of the appleseed material
        int         m_sides;    // sides of the object to which the material must be applied
    };

    MaterialInfo get_or_create_material(
        asr::Assembly&          assembly,
        asr::Assembly*          root_assembly,
        const std::string&      instance_name,
        Mtl*                    mtl,
        MaterialMap&            material_map,
        const bool              use_max_procedural_maps,
        const TimeValue         time)
    {
        MaterialInfo material_info;
        asr::Assembly* parent_assembly = root_assembly != nullptr ? root_assembly : &assembly;

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
                    make_unique_name(parent_assembly->materials(), wide_to_utf8(mtl->GetName()) + "_mat");
                parent_assembly->materials().insert(
                    appleseed_mtl->create_material(
                        *parent_assembly,
                        material_info.m_name.c_str(),
                        use_max_procedural_maps,
                        time));
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
            material_info.m_name = insert_empty_material(*parent_assembly, instance_name + "_mat");
            material_info.m_sides = asr::ObjectInstance::FrontSide | asr::ObjectInstance::BackSide;
        }

        return material_info;
    }

    void for_each_modifier(Object* object, const Class_ID modifier_class_id, const std::function<bool (Modifier* modifier)>& callback)
    {
        if (object->SuperClassID() == GEN_DERIVOB_CLASS_ID)
        {
            IDerivedObject* derived_object = static_cast<IDerivedObject*>(object);
            for (int i = 0, e = derived_object->NumModifiers(); i < e; ++i)
            {
                Modifier* modifier = derived_object->GetModifier(i);
                if (modifier->ClassID() == modifier_class_id)
                {
                    if (callback(modifier))
                        break;
                }
            }
        }
    }

    asr::VisibilityFlags::Type get_visibility_flags(Object* object, const TimeValue time)
    {
        asr::VisibilityFlags::Type flags = asr::VisibilityFlags::AllRays;

        for_each_modifier(object, AppleseedObjPropsMod::get_class_id(), [time, &flags](Modifier* modifier)
        {
            const auto obj_props_mod = static_cast<const AppleseedObjPropsMod*>(modifier);
            flags = obj_props_mod->get_visibility_flags(time);
            return true;
        });

        return flags;
    }

    std::string get_sss_set(Object* object, const TimeValue time)
    {
        std::string sss_set;

        for_each_modifier(object, AppleseedObjPropsMod::get_class_id(), [time, &sss_set](Modifier* modifier)
        {
            const auto obj_props_mod = static_cast<const AppleseedObjPropsMod*>(modifier);
            sss_set = obj_props_mod->get_sss_set(time);
            return true;
        });

        return sss_set;
    }

    int get_medium_priority(Object* object, const TimeValue time)
    {
        int medium_priority = 0;

        for_each_modifier(object, AppleseedObjPropsMod::get_class_id(), [time, &medium_priority](Modifier* modifier)
        {
            const auto obj_props_mod = static_cast<const AppleseedObjPropsMod*>(modifier);
            medium_priority = obj_props_mod->get_medium_priority(time);
            return true;
        });

        return medium_priority;
    }

    bool is_motion_blur_enabled(INode* node, const TimeValue time)
    {
        constexpr int ObjectMotionBlur = 1;
        return node->GetMotBlurOnOff(time) && node->MotBlur() == ObjectMotionBlur;
    }

    bool should_optimize_for_instancing(Object* object, const TimeValue time)
    {
        bool optimize_for_instancing = false;

        for_each_modifier(object, AppleseedObjPropsMod::get_class_id(), [time, &optimize_for_instancing](Modifier* modifier)
        {
            int value = 0;
            modifier->GetParamBlockByID(0)->GetValueByName(L"optimize_for_instancing", time, value, FOREVER);
            optimize_for_instancing = value == TRUE;
            return true;
        });

        return optimize_for_instancing;
    }

    bool is_photon_target(Object* object, const TimeValue time)
    {
        bool is_photon_target = false;

        for_each_modifier(object, AppleseedObjPropsMod::get_class_id(), [time, &is_photon_target](Modifier* modifier)
        {
            int value = 0;
            modifier->GetParamBlockByID(0)->GetValueByName(L"photon_target", time, value, FOREVER);
            is_photon_target = value == TRUE;
            return true;
        });

        return is_photon_target;
    }

    bool is_light_emitting_material(Mtl* mtl)
    {
        IAppleseedMtl* appleseed_mtl =
            static_cast<IAppleseedMtl*>(mtl->GetInterface(IAppleseedMtl::interface_id()));
        if (appleseed_mtl != nullptr && appleseed_mtl->can_emit_light())
            return true;
        else
        {
            const int submtl_count = mtl->NumSubMtls();
            for (int i = 0; i < submtl_count; ++i)
            {
                Mtl* sub_mtl = mtl->GetSubMtl(i);
                if (sub_mtl != nullptr)
                {
                    if (is_light_emitting_material(sub_mtl))
                        return true;
                }
            }
        }
        return false;
    }

    bool is_glass_material(Mtl* mtl)
    {
        return mtl != nullptr && mtl->ClassID() == AppleseedGlassMtl::get_class_id();
    }

    Mtl* override_material(Mtl* mtl, const RendererSettings& settings)
    {
        if (mtl == nullptr)
            return settings.m_override_material;

        if (settings.m_enable_override_material &&
            settings.m_override_material != nullptr)
        {
            if (is_light_emitting_material(mtl))
            {
                if (settings.m_override_exclude_light_materials)
                    return mtl;
                else
                    return settings.m_override_material;
            }

            if (is_glass_material(mtl))
            {
                if (settings.m_override_exclude_glass_materials)
                    return mtl;
                else
                    return settings.m_override_material;
            }

            return settings.m_override_material;
        }

        return mtl;
    }

    bool is_node_animated(INode* node)
    {
        while (true)
        {
            Control* tm_controller = node->GetTMController();
            if (tm_controller->GetPositionController()->IsAnimated() > 0 ||
                tm_controller->GetRotationController()->IsAnimated() > 0 ||
                tm_controller->GetScaleController()->IsAnimated() > 0)
                return true;

            if (node->IsRootNode())
                return false;

            node = node->GetParentNode();
        }
    }

    enum class RenderType
    {
        Default,
        MaterialPreview
    };

    void create_object_instance(
        asr::Assembly&          assembly,
        asr::Assembly*          root_assembly,
        INode*                  instance_node,
        const asf::Transformd&  transform,
        const ObjectInfo&       object_info,
        const RenderType        type,
        const RendererSettings& settings,
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

                    if (type != RenderType::MaterialPreview)
                        submtl = override_material(submtl, settings);

                    if (submtl != nullptr)
                    {
                        const auto material_info =
                            get_or_create_material(
                                assembly,
                                root_assembly,
                                instance_name,
                                submtl,
                                material_map,
                                settings.m_use_max_procedural_maps,
                                time);

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

                if (type != RenderType::MaterialPreview)
                    mtl = override_material(mtl, settings);

                // Create the appleseed material.
                const auto material_info =
                    get_or_create_material(
                        assembly,
                        root_assembly,
                        instance_name,
                        mtl,
                        material_map,
                        settings.m_use_max_procedural_maps,
                        time);

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

            if (type != RenderType::MaterialPreview)
                mtl = override_material(mtl, settings);

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

        // Retrieve the geometrical object referenced by this node.
        Object* object = instance_node->GetObjectRef();

        // Parameters.
        asr::ParamArray params;
        params.insert("visibility", asr::VisibilityFlags::to_dictionary(get_visibility_flags(object, time)));
        params.insert("sss_set_id", get_sss_set(object, time));
        params.insert("medium_priority", get_medium_priority(object, time));
        params.insert("photon_target", is_photon_target(object, time));
        if (type == RenderType::MaterialPreview)
            params.insert_path("visibility.shadow", false);

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
    typedef std::map<Object*, std::string> AssemblyMap;

    void add_object(
        asr::Assembly&          assembly,
        INode*                  node,
        INode*                  view_node,
        const RenderType        type,
        const RendererSettings& settings,
        const TimeValue         time,
        ObjectMap&              object_map,
        MaterialMap&            material_map,
        AssemblyMap&            assembly_map)
    {
        // Retrieve the geometrical object referenced by this node.
        Object* object = node->GetObjectRef();

        // Compute the transform of this instance.
        const asf::Transformd transform =
            asf::Transformd::from_local_to_parent(
                to_matrix4d(node->GetObjTMAfterWSM(time)));

        if (is_motion_blur_enabled(node, time) || should_optimize_for_instancing(object, time))
        {
            // Look for an existing assembly for that object, or create one if none could be found.
            std::string assembly_name;
            const AssemblyMap::const_iterator it = assembly_map.find(object);
            if (it == assembly_map.end())
            {
                // Create an assembly for that object.
                assembly_name = make_unique_name(assembly.assemblies(), wide_to_utf8(node->GetName()) + "_assembly");
                asf::auto_release_ptr<asr::Assembly> object_assembly(
                    asr::AssemblyFactory().create(assembly_name.c_str()));

                // Add objects and object instances to that assembly.
                const auto object_infos = create_objects(object_assembly.ref(), node, time);
                for (const auto& object_info : object_infos)
                {
                    create_object_instance(
                        object_assembly.ref(),
                        &assembly,
                        node,
                        asf::Transformd::identity(),
                        object_info,
                        type,
                        settings,
                        time,
                        material_map);
                }

                // Remember the name of the assembly corresponding to that object.
                assembly_map.insert(std::make_pair(object, assembly_name));
                    
                // Insert the assembly into the scene.
                assembly.assemblies().insert(object_assembly);
            }
            else
            {
                assembly_name = it->second;
            }

            // Create an instance of the assembly corresponding to that object.
            const std::string object_assembly_instance_name =
                make_unique_name(assembly.assembly_instances(), assembly_name + "_instance");
            asf::auto_release_ptr<asr::AssemblyInstance> object_assembly_instance(
                asr::AssemblyInstanceFactory::create(
                    object_assembly_instance_name.c_str(),
                    asr::ParamArray(),
                    assembly_name.c_str()));
            object_assembly_instance->transform_sequence().set_transform(0.0, transform);

            // Apply transformation motion blur if enabled on that object.
            if (is_motion_blur_enabled(node, time))
            {
                object_assembly_instance->transform_sequence()
                    .set_transform(1.0, asf::Transformd::from_local_to_parent(
                        to_matrix4d(node->GetObjTMAfterWSM(time + GetTicksPerFrame()))));
            }

            // Insert the assembly instance into the parent assembly.
            assembly.assembly_instances().insert(object_assembly_instance);
        }
        else
        {
            // Check if we already generated the corresponding appleseed objects.
            ObjectMap::const_iterator it = object_map.find(object);
            if (it == object_map.end())
            {
                // Create appleseed objects.
                const auto object_infos = create_objects(assembly, node, time);
                it = object_map.insert(std::make_pair(object, object_infos)).first;
            }

            // Create object instances.
            for (const auto& object_info : it->second)
            {
                create_object_instance(
                    assembly,
                    nullptr,
                    node,
                    transform,
                    object_info,
                    type,
                    settings,
                    time,
                    material_map);
            }
        }
    }

    void add_objects(
        asr::Assembly&          assembly,
        INode*                  view_node,
        const MaxSceneEntities& entities,
        const RenderType        type,
        const RendererSettings& settings,
        const TimeValue         time,
        ObjectMap&              object_map,
        MaterialMap&            material_map,
        AssemblyMap&            assembly_map,
        RendProgressCallback*   progress_cb)
    {
        for (size_t i = 0, e = entities.m_objects.size(); i < e; ++i)
        {
            const auto& object = entities.m_objects[i];
            add_object(
                assembly,
                object,
                view_node,
                type,
                settings,
                time,
                object_map,
                material_map,
                assembly_map);

            const int done = static_cast<int>(i);
            const int total = static_cast<int>(e);
            if (progress_cb->Progress(done + 1, total) == RENDPROG_ABORT)
                break;
        }
    }


    void add_point_light(
        asr::Assembly&          assembly,
        const std::string&      light_name,
        const asf::Transformd&  transform,
        const std::string&      color_name,
        const float             intensity)
    {
        asf::auto_release_ptr<asr::Light> light(
            asr::PointLightFactory().create(
                light_name.c_str(),
                asr::ParamArray()
                    .insert("intensity", color_name)
                    .insert("intensity_multiplier", intensity * asf::Pi<float>())));
        light->set_transform(transform);
        assembly.lights().insert(light);
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

    void add_light_instance(
        asr::Assembly&                      assembly,
        const std::string&                  light_name,
        const Matrix3&                      light_transform,
        const std::string&                  color_name,
        const float                         intensity,
        const asr::VisibilityFlags::Type    visibility)
    {
        // EDF for the light.
        const std::string edf_name = make_unique_name(assembly.edfs(), light_name + "_edf");
        assembly.edfs().insert(
            asr::DiffuseEDFFactory().create(edf_name.c_str(), 
                asr::ParamArray()
                    .insert("radiance", color_name)
                    .insert("radiance_multiplier", intensity)));

        // Material for the light.
        const std::string material_name =
            make_unique_name(assembly.materials(), light_name + "_mat");
        assembly.materials().insert(
            asr::GenericMaterialFactory().create(
                material_name.c_str(), 
                asr::ParamArray().insert("edf", edf_name)));

        // Light object instance.
        const std::string instance_name =
            make_unique_name(assembly.object_instances(), light_name + "_inst");

        asf::StringDictionary front_material_mappings;
        front_material_mappings.insert("default", material_name);

        // Create the instance and insert it into the assembly.
        const asf::Transformd transform = asf::Transformd::from_local_to_parent(to_matrix4d(light_transform));
        assembly.object_instances().insert(
            asr::ObjectInstanceFactory::create(
                instance_name.c_str(),
                asr::ParamArray()
                    .insert(
                        "visibility",
                        asr::VisibilityFlags::to_dictionary(visibility)),
                light_name.c_str(),
                transform,
                front_material_mappings));
    }

    typedef LightscapeLight::LightTypes LightType;

    void add_physical_light(
        asr::Assembly&          assembly,
        INode*                  light_node,
        LightscapeLight*        light_object,
        const asf::Color3f      color,
        const float             intensity,
        const TimeValue         time)
    {
        const ObjectState object_state = light_node->EvalWorldState(time);
        
        // Compute a unique name for this light.
        std::string object_name = wide_to_utf8(light_node->GetName());
        object_name = make_unique_name(assembly.objects(), object_name);
        const std::string color_name = insert_color(assembly, object_name + "_color", color);

        Matrix3 light_transform = light_node->GetObjTMAfterWSM(time);
        light_transform.PreRotateX(-asf::HalfPi<float>());

        asf::auto_release_ptr<asr::Object> light_obj;

        const int light_type = light_object->Type();

        switch (light_type)
        {
          case LightType::TARGET_SPHERE_TYPE:
          case LightType::SPHERE_TYPE:
            {
                light_obj = asr::SphereObjectFactory().create(object_name.c_str(),
                    asr::ParamArray()
                        .insert("radius", light_object->GetRadius(time))
                );
            }
            break;

          case LightType::TARGET_AREA_TYPE:
          case LightType::AREA_TYPE:
            {
                light_obj = asr::RectangleObjectFactory().create(object_name.c_str(),
                    asr::ParamArray()
                        .insert("width", light_object->GetWidth(time))
                        .insert("height", light_object->GetLength(time))
                );
            }
            break;

          case LightType::TARGET_DISC_TYPE:
          case LightType::DISC_TYPE:
            {
                light_obj = asr::DiskObjectFactory().create(object_name.c_str(),
                    asr::ParamArray()
                        .insert("radius", light_object->GetRadius(time))
                );
            }
            break;

          default:
            break;
        }

        if (light_obj.get() != nullptr)
        {
            // Light Object.
            assembly.objects().insert(light_obj);

            const asr::VisibilityFlags::Type visibility =
                get_visibility_flags(light_node->GetObjectRef(), time);
            
            add_light_instance(
                assembly,
                object_name,
                light_transform,
                color_name,
                intensity,
                visibility);
        }
        else
        {
            // Unsupported physical light type.
            // todo: emit warning message.
        }
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
        if (light_object == nullptr)
            return;

        const asf::Color3f color = to_color3f(light_object->GetRGBColor(time));
        const float intensity = light_object->GetIntensity(time);
        const float decay_start = light_object->GetDecayRadius(time);
        const int decay_exponent = light_object->GetDecayType();

        // Skip exporting lights with zero intensity
        if (!asf::is_zero(color) && intensity > 0.0f)
        {
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
                env_map->GetParamBlock(0)->GetValueByName(L"sun_node", time, sun_node, FOREVER);
                env_map->GetParamBlock(0)->GetValueByName(L"sun_node_on", time, sun_node_on, FOREVER);
                env_map->GetParamBlock(0)->GetValueByName(L"sun_size_multiplier", time, sun_size_mult, FOREVER);
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
            else if (object_state.obj->IsSubClassOf(LIGHTSCAPE_LIGHT_CLASS))
            {
                LightscapeLight* light_object = dynamic_cast<LightscapeLight*>(object_state.obj);
                if (light_object == nullptr)
                    return;
                
                const asf::Color3f physical_color = to_color3f(light_object->GetRGBFilter(time));
                const float physical_intensity = light_object->GetResultingIntensity(time);
                if (asf::is_zero(physical_color) || physical_intensity == 0.0f)
                    return;

                const int light_type = light_object->Type();

                if (light_type == LightType::TARGET_POINT_TYPE ||
                    light_type == LightType::POINT_TYPE)
                {
                    add_point_light(
                        assembly,
                        light_name,
                        transform,
                        color_name,
                        physical_intensity);
                }
                else
                {
                    add_physical_light(
                        assembly,
                        light_node,
                        light_object,
                        physical_color,
                        physical_intensity,
                        time);
                }
            }
            else
            {
                // Unsupported light type.
                // todo: emit warning message.
            }
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
                // Unsupported light type.
                // todo: emit warning message.
                break;
            }
        }
    }

    bool has_light_emitting_materials(const MaterialMap& material_map)
    {
        for (const auto& entry : material_map)
        {
            Mtl* mtl = entry.first;
            if (is_light_emitting_material(mtl))
                return true;
        }

        return false;
    }

    void populate_assembly(
        asr::Scene&                         scene,
        asr::Assembly&                      assembly,
        INode*                              view_node,
        const RendParams&                   rend_params,
        const MaxSceneEntities&             entities,
        const std::vector<DefaultLight>&    default_lights,
        const RenderType                    type,
        const RendererSettings&             settings,
        const TimeValue                     time,
        RendProgressCallback*               progress_cb)
    {
        // Add objects, object instances and materials to the assembly.
        ObjectMap object_map;
        MaterialMap material_map;
        AssemblyMap assembly_map;
        add_objects(
            assembly,
            view_node,
            entities,
            type,
            settings,
            time,
            object_map,
            material_map,
            assembly_map,
            progress_cb);

        // Only add non-physical lights. Light-emitting materials were added by material plugins.
        add_lights(assembly, rend_params, entities, time);

        // Add Max's default lights if
        //       the scene does not contain non-physical lights (point lights, spot lights, etc.)
        //   and the scene does not contain light-emitting materials
        //   and the scene does not contain a light-emitting environment
        //   and checkbox Force Off Default Lights is off
        const bool has_lights = !entities.m_lights.empty();
        const bool has_emitting_mats = has_light_emitting_materials(material_map);
        const bool has_emitting_env = !scene.get_environment()->get_parameters().get_optional<std::string>("environment_edf").empty();
        if (rend_params.inMtlEdit ||
            (!has_lights &&
             !has_emitting_mats &&
             !(has_emitting_env && settings.m_background_emits_light) &&
             !settings.m_force_off_default_lights))
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
            auto env_map = appleseed_envmap->create_envmap("environment_edf", time);
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
                        rend_params.envMap,
                        time);
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

            if (is_bitmap_texture(rend_params.envMap))
            {
                TextureOutput* tex_out = static_cast<BitmapTex*>(rend_params.envMap)->GetTexout();
                if (tex_out)
                {
                    const float output = static_cast<StdTexoutGen*>(tex_out)->GetOutAmt(time);
                    env_edf_params.insert("exposure", output - 1.0f);
                }
            }

            asf::Vector3d uv_flip_vec(-1.0, 1.0, 1.0);
            UVGen* uvgen = rend_params.envMap->GetTheUVGen();
            if (uvgen && uvgen->IsStdUVGen())
            {
                StdUVGen* std_uvgen = static_cast<StdUVGen*>(uvgen);

                const float h_shift = std_uvgen->GetUOffs(time) * 360.0f + 90.0f + asf::rad_to_deg(std_uvgen->GetUAng(time));
                const float v_shift = std_uvgen->GetVOffs(time) * -180.0f - asf::rad_to_deg(std_uvgen->GetVAng(time));

                env_edf_params.insert("horizontal_shift", h_shift);
                env_edf_params.insert("vertical_shift", v_shift);

                uv_flip_vec = asf::Vector3d(-1.0 * std_uvgen->GetUScl(time), std_uvgen->GetVScl(time), 1.0);
            }

            // Flip environment horizontally to be consistent with Max environment background viewport preview.
            const asf::Transformd flip_lr =
                asf::Transformd::from_local_to_parent(
                    asf::Matrix4d::make_scaling(uv_flip_vec));

            asf::auto_release_ptr<asr::EnvironmentEDF> env_edf(
                asr::LatLongMapEnvironmentEDFFactory().create(
                    "environment_edf",
                    env_edf_params));

            env_edf->transform_sequence().set_transform(0.0f, flip_lr);
            scene.environment_edfs().insert(env_edf);
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

    const char* get_filter_type(const int filter_type)
    {
        switch (filter_type)
        {
          case 0:  return "blackman-harris";
          case 1:  return "box";
          case 2:  return "catmull";
          case 3:  return "bspline";
          case 4:  return "gaussian";
          case 5:  return "lanczos";
          case 6:  return "mitchell";
          case 7:  return "triangle";
          default: return "box";
        }
    }

    const char* get_denoise_mode(const int denoise_mode)
    {
        switch (denoise_mode)
        {
          case 0:  return "off";
          case 1:  return "on";
          case 2:  return "write_outputs";
          default: return "off";
        }
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
                        .insert("filter", "box")
                        .insert("filter_size", 0.5));
        }
        else
        {
            asr::AOVContainer aovs;

            IRenderElementMgr* re_manager = GetCOREInterface()->GetCurRenderElementMgr();
            if (re_manager != nullptr)
            {
                re_manager->SetDisplayElements(false);
                for (int i = 0, e = re_manager->NumRenderElements(); i < e; ++i)
                {
                    auto factories = g_aov_factory_registrar.get_factories();

                    auto* render_element = re_manager->GetRenderElement(i);
                    render_element->SetEnabled(false);

                    if (render_element->ClassID() == AppleseedRenderElement::get_class_id())
                    {
                        AppleseedRenderElement* element = static_cast<AppleseedRenderElement*>(render_element);
                        int aov_index = 0;
                        element->GetParamBlock(0)->GetValueByName(L"aov_index", 0, aov_index, FOREVER);
                        if (aov_index > 0 && aov_index <= static_cast<int>(factories.size()))
                        {
                            asf::auto_release_ptr<asr::AOV> aov_entity = factories[aov_index - 1]->create(asr::ParamArray());
                            if (aovs.get_by_name(aov_entity->get_name()) == nullptr)
                            {
                                PBBitmap* p_bitmap = nullptr;
                                render_element->GetPBBitmap(p_bitmap);
                                if (p_bitmap != nullptr)
                                {
                                    aov_entity->get_parameters().insert(
                                        "output_filename",
                                        wide_to_utf8(p_bitmap->bi.Name()).c_str());
                                }
                                aovs.insert(aov_entity);
                            }
                        }
                    }
                }
            }

            asf::auto_release_ptr<asr::Frame> frame(
                asr::FrameFactory::create(
                    "beauty",
                    asr::ParamArray()
                        .insert("camera", "camera")
                        .insert("resolution", asf::Vector2i(bitmap->Width(), bitmap->Height()))
                        .insert("tile_size", asf::Vector2i(settings.m_tile_size))
                        .insert("filter", get_filter_type(settings.m_pixel_filter))
                        .insert("filter_size", settings.m_pixel_filter_size)
                        .insert("denoiser", get_denoise_mode(settings.m_denoise_mode))
                        .insert("skip_denoised", settings.m_enable_skip_denoised)
                        .insert("prefilter_spikes", settings.m_enable_prefilter_spikes)
                        .insert("random_pixel_order", settings.m_enable_random_pixel_order)
                        .insert("patch_distance_threshold", settings.m_patch_distance_threshold)
                        .insert("spike_threshold", settings.m_spike_threshold)
                        .insert("denoise_scales", settings.m_denoise_scales)
                        .insert("noise_seed", settings.m_noise_seed),
                    aovs));

            if (rend_params.rendType == RENDTYPE_REGION)
            {
                frame->set_crop_window(
                    asf::AABB2u(
                        asf::Vector2u(frame_rend_params.regxmin, frame_rend_params.regymin),
                        asf::Vector2u(frame_rend_params.regxmax, frame_rend_params.regymax)));
            }

            if (settings.m_enable_render_stamp)
            {
                frame->post_processing_stages().insert(
                    asr::RenderStampPostProcessingStageFactory().create(
                        "render_stamp",
                        asr::ParamArray()
                            .insert("order", 0)
                            .insert("format_string", wide_to_utf8(settings.m_render_stamp_format))));
            }

            return frame;
        }
    }
}

void set_camera_film_params(
    asr::ParamArray&            params,
    MaxSDK::IPhysicalCamera*    camera_node,
    Bitmap*                     bitmap,
    const RendererSettings&     settings,
    const TimeValue             time)
{
    const float aspect = static_cast<float>(bitmap->Height()) / bitmap->Width();
    const float film_width = camera_node->GetFilmWidth(time, FOREVER) * settings.m_scale_multiplier;
    const float film_height = film_width * aspect;
    params.insert("film_dimensions", asf::Vector2f(film_width, film_height));
}

void set_camera_dof_params(
    asr::ParamArray&            params,
    MaxSDK::IPhysicalCamera*    camera_node,
    Bitmap*                     bitmap,
    const TimeValue             time)
{
    params.insert("f_stop", camera_node->GetLensApertureFNumber(time, FOREVER));
    params.insert("autofocus_enabled", false);
    params.insert("focal_distance", camera_node->GetFocusDistance(time, FOREVER));

    switch (camera_node->GetBokehShape(time, FOREVER))
    {
      case MaxSDK::IPhysicalCamera::BokehShape::Circular:
        params.insert("diaphragm_blades", 0);
        break;
      case MaxSDK::IPhysicalCamera::BokehShape::Bladed:
        params.insert("diaphragm_blades", camera_node->GetBokehNumberOfBlades(time, FOREVER));
        params.insert("diaphragm_tilt_angle", camera_node->GetBokehBladesRotationDegrees(time, FOREVER));
        break;
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

        // Look for a physical camera in the scene.
        MaxSDK::IPhysicalCamera* phys_camera = nullptr;
        if (view_node)
            phys_camera = dynamic_cast<MaxSDK::IPhysicalCamera*>(view_node->EvalWorldState(time).obj);

        if (phys_camera != nullptr)
        {
            // Film dimensions.
            set_camera_film_params(params, phys_camera, bitmap, settings, time);

            if (phys_camera->GetDOFEnabled(time, FOREVER))
            {   
                set_camera_dof_params(params, phys_camera, bitmap, time); 

                // Create camera - DOF enabled.
                camera = asr::ThinLensCameraFactory().create("camera", params);
            }
            else
            {
                // Create camera - DOF disabled.
                camera = asr::PinholeCameraFactory().create("camera", params);
            }

            if (phys_camera->GetMotionBlurEnabled(time, FOREVER))
            {
                const asf::Transformd transform =
                    asf::Transformd::from_local_to_parent(
                        asf::Matrix4d::make_scaling(asf::Vector3d(settings.m_scale_multiplier)) *
                        to_matrix4d(view_node->GetObjTMAfterWSM(time + GetTicksPerFrame())));

                camera->transform_sequence().set_transform(1.0, transform);

                // Set motion blur parameters.
                asr::ParamArray& camera_params = camera->get_parameters();
                const float opening_time = phys_camera->GetShutterOffsetInFrames(time, FOREVER);
                const float closing_time = opening_time + phys_camera->GetShutterDurationInFrames(time, FOREVER);
                camera_params.insert("shutter_open_begin_time", opening_time);
                camera_params.insert("shutter_open_end_time", opening_time);
                camera_params.insert("shutter_close_begin_time", closing_time);
                camera_params.insert("shutter_close_end_time", closing_time);
            }
        }
        else
        {
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
    const TimeValue                         time,
    RendProgressCallback*                   progress_cb)
{
    // Create an empty project.
    asf::auto_release_ptr<asr::Project> project(
        asr::ProjectFactory::create("project"));

    // Initialize search paths.
    project->search_paths().set_root_path(get_root_path());
    project->search_paths().push_back_explicit_path("shaders/max");
    project->search_paths().push_back_explicit_path("shaders/appleseed");
    project->search_paths().push_back_explicit_path(".");

    // Discover and load plugins before building the scene.
    project->get_plugin_store().load_all_plugins_from_paths(project->search_paths());

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
        view_node,
        rend_params,
        entities,
        default_lights,
        type,
        settings,
        time,
        progress_cb);

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
