
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2015-2016 Francois Beaune, The appleseedhq Organization
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
#include "renderer/maxsceneentities.h"
#include "renderer/renderersettings.h"
#include "iappleseedmtl.h"
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
#include "foundation/image/image.h"
#include "foundation/math/scalar.h"
#include "foundation/math/transform.h"
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
#include <object.h>
#include <triobj.h>

// Standard headers.
#include <cstddef>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <utility>

namespace asf = foundation;
namespace asr = renderer;

namespace
{
    template <typename EntityContainer>
    std::string make_unique_name(
        const EntityContainer&  entities,
        const std::string&      name)
    {
        return
            entities.get_by_name(name.c_str()) == nullptr
                ? name
                : asr::make_unique_name(name + "_", entities);
    }

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
            asr::GenericMaterialFactory::static_create(
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
            asr::DisneyMaterialFactory::static_create(
                name.c_str(),
                asr::ParamArray()));

        // The Disney material expects sRGB colors, so we have to convert the input color to sRGB.
        static_cast<asr::DisneyMaterial*>(material.get())->add_layer(
            asr::DisneyMaterialLayer::get_default_values()
                .insert("base_color", fmt_expr(asf::linear_rgb_to_srgb(linear_rgb)))
                .insert("specular", 1.0)
                .insert("roughness", 0.625));

        assembly.materials().insert(material);

        return name;
    }

    TriObject* get_tri_object_from_node(
        const ObjectState&      object_state,
        const TimeValue         time,
        bool&                   must_delete)
    {
        DbgAssert(object_state.obj);

        must_delete = false;

        const Class_ID TriObjectClassID(TRIOBJ_CLASS_ID, 0);

        if (!object_state.obj->CanConvertToType(TriObjectClassID))
            return nullptr;

        TriObject* tri_object = static_cast<TriObject*>(object_state.obj->ConvertToType(time, TriObjectClassID));
        must_delete = tri_object != object_state.obj;

        return tri_object;
    }

    bool create_mesh_object(
        asr::Assembly&          assembly,
        INode*                  object_node,
        const TimeValue         time,
        std::string&            object_name)
    {
        // Compute a unique name for the instantiated object.
        object_name = wide_to_utf8(object_node->GetName());
        object_name = make_unique_name(assembly.objects(), object_name);

        // Retrieve the ObjectState at the desired time.
        const ObjectState object_state = object_node->EvalWorldState(time);

        // Convert the object to a TriObject.
        bool must_delete_tri_object;
        TriObject* tri_object = get_tri_object_from_node(object_state, time, must_delete_tri_object);
        if (tri_object == nullptr)
        {
            // todo: emit warning message.
            return false;
        }

        // Create a new mesh object.
        asf::auto_release_ptr<asr::MeshObject> object(
            asr::MeshObjectFactory::create(object_name.c_str(), asr::ParamArray()));
        {
            Mesh& mesh = tri_object->GetMesh();

            // Make sure the input mesh has vertex normals.
            mesh.checkNormals(TRUE);

            // Create a material slot.
            const asf::uint32 material_slot =
                static_cast<asf::uint32>(object->push_material_slot("material"));

            // Copy vertices to the mesh object.
            object->reserve_vertices(mesh.getNumVerts());
            for (int i = 0, e = mesh.getNumVerts(); i < e; ++i)
            {
                const Point3& v = mesh.getVert(i);
                object->push_vertex(asr::GVector3(v.x, v.y, v.z));
            }

            // Copy texture vertices to the mesh object.
            object->reserve_tex_coords(mesh.getNumTVerts());
            for (int i = 0, e = mesh.getNumTVerts(); i < e; ++i)
            {
                const UVVert& uv = mesh.getTVert(i);
                object->push_tex_coords(asr::GVector2(uv.x, uv.y));
            }

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
                    const Point3& n = mesh.getFaceNormal(i);
                    const asf::uint32 normal_index =
                        static_cast<asf::uint32>(
                            object->push_vertex_normal(asr::GVector3(n.x, n.y, n.z)));
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
                                    object->push_vertex_normal(asr::GVector3(n.x, n.y, n.z)));
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
                                            object->push_vertex_normal(asr::GVector3(n.x, n.y, n.z)));
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
                triangle.m_pa = material_slot;

                object->push_triangle(triangle);
            }

            // Delete the TriObject if necessary.
            if (must_delete_tri_object)
                tri_object->DeleteMe();

            // todo: optimize the object.
        }

        // Insert the object into the assembly.
        assembly.objects().insert(asf::auto_release_ptr<asr::Object>(object));

        return true;
    }

    typedef std::map<Mtl*, std::string> MaterialMap;

    enum class RenderType
    {
        Default,
        MaterialPreview
    };

    void create_object_instance(
        asr::Assembly&          assembly,
        INode*                  instance_node,
        const std::string&      object_name,
        const RenderType        type,
        const TimeValue         time,
        MaterialMap&            material_map)
    {
        // Compute a unique name for this instance.
        const std::string instance_name =
            make_unique_name(assembly.object_instances(), object_name + "_inst");

        // Compute the transform of this instance.
        const asf::Transformd transform =
            asf::Transformd::from_local_to_parent(
                to_matrix4d(instance_node->GetObjTMAfterWSM(time)));

        // Name of the material of this instance.
        std::string material_name;
        int material_sides = 0;

        // Retrieve or create an appleseed material.
        Mtl* mtl = instance_node->GetMtl();
        if (mtl)
        {
            // The instance has a material.
            IAppleseedMtl* appleseed_mtl =
                static_cast<IAppleseedMtl*>(mtl->GetInterface(IAppleseedMtl::interface_id()));
            if (appleseed_mtl)
            {
                // The instance has an appleseed material.
                const MaterialMap::const_iterator it = material_map.find(mtl);
                if (it == material_map.end())
                {
                    // The appleseed material does not exist yet, let the material plugin create it.
                    material_name =
                        make_unique_name(assembly.materials(), wide_to_utf8(mtl->GetName()) + "_mat");
                    assembly.materials().insert(
                        appleseed_mtl->create_material(assembly, material_name.c_str()));
                    material_map.insert(std::make_pair(mtl, material_name));
                }
                else
                {
                    // The appleseed material exists.
                    material_name = it->second;
                }
                material_sides = appleseed_mtl->get_sides();
            }
            else
            {
                // The instance has a non-appleseed material: assign it an empty material that will appear black.
                material_name = insert_empty_material(assembly, instance_name + "_mat");
                material_sides = asr::ObjectInstance::FrontSide | asr::ObjectInstance::BackSide;
            }
        }
        else
        {
            // The instance does not have a material: create a new default material.
            material_name =
                insert_default_material(
                    assembly,
                    instance_name + "_mat",
                    to_color3f(Color(instance_node->GetWireColor())));
            material_sides = asr::ObjectInstance::FrontSide | asr::ObjectInstance::BackSide;
        }

        // Parameters.
        asr::ParamArray params;
        if (type == RenderType::MaterialPreview)
            params.insert_path("visibility.shadow", false);

        // Material mappings.
        asf::StringDictionary front_material_mappings;
        if (material_sides & asr::ObjectInstance::FrontSide)
            front_material_mappings.insert("material", material_name);
        asf::StringDictionary back_material_mappings;
        if (material_sides & asr::ObjectInstance::BackSide)
            back_material_mappings.insert("material", material_name);

        // Create the instance and insert it into the assembly.
        assembly.object_instances().insert(
            asr::ObjectInstanceFactory::create(
                instance_name.c_str(),
                params,
                object_name.c_str(),
                transform,
                front_material_mappings,
                back_material_mappings));
    }

    // Information about an appleseed object.
    struct ObjectInfo
    {
        bool        m_valid;    // the object was successfully generated
        std::string m_name;     // name of the object
    };

    typedef std::map<Object*, ObjectInfo> ObjectMap;

    void add_object(
        asr::Assembly&          assembly,
        INode*                  node,
        const RenderType        type,
        const TimeValue         time,
        ObjectMap&              object_map,
        MaterialMap&            material_map)
    {
        // Retrieve the geometrical object referenced by this node.
        Object* object = node->GetObjectRef();

        // Check if we already generated the corresponding appleseed object.
        const ObjectMap::const_iterator it = object_map.find(object);
        ObjectInfo object_info;

        if (it == object_map.end())
        {
            // The appleseed object does not exist yet, create it.
            object_info.m_valid =
                create_mesh_object(
                    assembly,
                    node,
                    time,
                    object_info.m_name);
            object_map.insert(std::make_pair(object, object_info));
        }
        else
        {
            // The appleseed object exists, retrieve its info.
            object_info = it->second;
        }

        // Create an instance of this object.
        if (object_info.m_valid)
        {
            create_object_instance(
                assembly,
                node,
                object_info.m_name,
                type,
                time,
                material_map);
        }
    }

    void add_objects(
        asr::Assembly&          assembly,
        const MaxSceneEntities& entities,
        const RenderType        type,
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
            asr::MaxOmniLightFactory::static_create(
                light_name.c_str(),
                asr::ParamArray()
                    .insert("intensity", color_name)
                    .insert("intensity_multiplier", intensity * asf::Pi)
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
            asr::MaxSpotLightFactory::static_create(
                light_name.c_str(),
                asr::ParamArray()
                    .insert("intensity", color_name)
                    .insert("intensity_multiplier", intensity * asf::Pi)
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
            asr::DirectionalLightFactory::static_create(
                light_name.c_str(),
                asr::ParamArray()
                    .insert("irradiance", color_name)
                    .insert("irradiance_multiplier", intensity * asf::Pi)));
        light->set_transform(transform);
        assembly.lights().insert(light);
    }

    void add_light(
        asr::Assembly&          assembly,
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

        if (light_object->ClassID() == Class_ID(OMNI_LIGHT_CLASS_ID, 0))
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
        const MaxSceneEntities& entities,
        const TimeValue         time)
    {
        for (const auto& light_info : entities.m_lights)
        {
            if (light_info.m_enabled)
                add_light(assembly, light_info.m_light, time);
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
        asr::Assembly&                      assembly,
        const MaxSceneEntities&             entities,
        const std::vector<DefaultLight>&    default_lights,
        const RenderType                    type,
        const TimeValue                     time)
    {
        ObjectMap object_map;
        MaterialMap material_map;

        // Add objects, and consequently object instances and materials, to the assembly.
        add_objects(assembly, entities, type, time, object_map, material_map);

        // Only add non-physical lights. Light-emitting materials were added by material plugins.
        add_lights(assembly, entities, time);

        if (entities.m_lights.empty() && !has_light_emitting_materials(material_map))
        {
            // No non-physical light or light-emitting material: add Max's default lights.
            add_default_lights(assembly, default_lights);
        }
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
            const size_t TextureWidth = 512;
            const size_t TextureHeight = 512;

            // Render the environment map into a Max bitmap.
            BitmapInfo bi;
            bi.SetWidth(TextureWidth);
            bi.SetHeight(TextureHeight);
            bi.SetType(BMM_FLOAT_RGBA_32);
            Bitmap* envmap_bitmap = TheManager->Create(&bi);
            rend_params.envMap->RenderBitmap(time, envmap_bitmap, 1.0f, TRUE);

            // Build an appleseed image from the Max bitmap.
            asf::auto_release_ptr<asf::Image> envmap_image(
                new asf::Image(
                    TextureWidth, TextureHeight,    // image dimensions
                    TextureWidth, TextureHeight,    // tile dimensions
                    4,
                    asf::PixelFormatFloat));
            for (size_t y = 0; y < TextureHeight; ++y)
            {
                for (size_t x = 0; x < TextureWidth; ++x)
                {
                    BMM_Color_fl c;
                    envmap_bitmap->GetLinearPixels(
                        static_cast<int>(x),
                        static_cast<int>(y),
                        1,
                        &c);
                    envmap_image->set_pixel(x, y, c);
                }
            }

            // Destroy the Max bitmap.
            envmap_bitmap->DeleteThis();

            scene.textures().insert(
                asf::auto_release_ptr<asr::Texture>(
                    asr::MemoryTexture2dFactory::static_create(
                        "environment_map",
                        asr::ParamArray()
                            .insert("color_space", "linear_rgb"),
                        envmap_image)));

            scene.texture_instances().insert(
                asf::auto_release_ptr<asr::TextureInstance>(
                    asr::TextureInstanceFactory::create(
                        "environment_map_inst",
                        asr::ParamArray(),
                        "environment_map")));

            scene.environment_edfs().insert(
                asf::auto_release_ptr<asr::EnvironmentEDF>(
                    asr::LatLongMapEnvironmentEDFFactory::static_create(
                        "environment_edf",
                        asr::ParamArray()
                            .insert("radiance", "environment_map_inst"))));

            scene.environment_shaders().insert(
                asr::BackgroundEnvironmentShaderFactory::static_create(
                    "environment_shader",
                    asr::ParamArray()
                        .insert("color", "environment_map_inst")));

            scene.set_environment(
                asr::EnvironmentFactory::create(
                    "environment",
                    asr::ParamArray()
                        .insert("environment_edf", "environment_edf")
                        .insert("environment_shader", "environment_shader")));
        }
        else
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
                    asr::ConstantEnvironmentEDFFactory::static_create(
                        "environment_edf",
                        asr::ParamArray()
                            .insert("radiance", background_color_name)));

                scene.environment_shaders().insert(
                    asr::EDFEnvironmentShaderFactory::static_create(
                        "environment_shader",
                        asr::ParamArray()
                            .insert("environment_edf", "environment_edf")));

                if (settings.m_background_emits_light)
                {
                    scene.set_environment(
                        asr::EnvironmentFactory::create(
                            "environment",
                            asr::ParamArray()
                                .insert("environment_edf", "environment_edf")
                                .insert("environment_shader", "environment_shader")));
                }
                else
                {
                    scene.set_environment(
                        asr::EnvironmentFactory::create(
                            "environment",
                            asr::ParamArray()
                                .insert("environment_shader", "environment_shader")));
                }
            }
        }
    }

    asf::auto_release_ptr<asr::Camera> build_camera(
        const ViewParams&       view_params,
        Bitmap*                 bitmap,
        const TimeValue         time)
    {
        asr::ParamArray params;

        if (view_params.projType == PROJ_PERSPECTIVE)
        {
            params.insert("film_dimensions", asf::Vector2i(bitmap->Width(), bitmap->Height()));
            params.insert("horizontal_fov", asf::rad_to_deg(view_params.fov));
        }
        else
        {
            DbgAssert(view_params.projType == PROJ_PARALLEL);

            const float ViewDefaultWidth = 400.0f;
            const float aspect = static_cast<float>(bitmap->Height()) / bitmap->Width();
            const float film_width = ViewDefaultWidth * view_params.zoom;
            const float film_height = film_width * aspect;
            params.insert("film_dimensions", asf::Vector2f(film_width, film_height));
        }

        params.insert("near_z", -view_params.hither);

        asf::auto_release_ptr<renderer::Camera> camera =
            view_params.projType == PROJ_PERSPECTIVE
                ? asr::PinholeCameraFactory::static_create("camera", params)
                : asr::OrthographicCameraFactory::static_create("camera", params);

        camera->transform_sequence().set_transform(
            0.0, asf::Transformd::from_local_to_parent(
                to_matrix4d(Inverse(view_params.affineTM))));

        return camera;
    }

    asf::auto_release_ptr<asr::Frame> build_frame(
        const asr::Camera&      camera,
        Bitmap*                 bitmap,
        const RendParams&       rend_params)
    {
        if (rend_params.inMtlEdit)
        {
            return
                asr::FrameFactory::create(
                    "beauty",
                    asr::ParamArray()
                        .insert("camera", camera.get_name())
                        .insert("resolution", asf::Vector2i(bitmap->Width(), bitmap->Height()))
                        .insert("tile_size", asf::Vector2i(8, 8))
                        .insert("color_space", "linear_rgb")
                        .insert("filter", "box")
                        .insert("filter_size", 0.5));
        }
        else
        {
            return
                asr::FrameFactory::create(
                    "beauty",
                    asr::ParamArray()
                        .insert("camera", camera.get_name())
                        .insert("resolution", asf::Vector2i(bitmap->Width(), bitmap->Height()))
                        .insert("tile_size", asf::Vector2i(64, 64))
                        .insert("color_space", "linear_rgb")
                        .insert("filter", "blackman-harris")
                        .insert("filter_size", 1.5));
        }
    }
}

asf::auto_release_ptr<asr::Project> build_project(
    const MaxSceneEntities&                 entities,
    const std::vector<DefaultLight>&        default_lights,
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

    // Add default configurations to the project.
    project->add_default_configurations();

    // Create a scene.
    asf::auto_release_ptr<asr::Scene> scene(asr::SceneFactory::create());

    // Create an assembly.
    asf::auto_release_ptr<asr::Assembly> assembly(
        asr::AssemblyFactory::static_create("assembly"));

    // Populate the assembly with entities from the 3ds Max scene.
    const RenderType type =
        rend_params.inMtlEdit ? RenderType::MaterialPreview : RenderType::Default;
    populate_assembly(
        assembly.ref(),
        entities,
        default_lights,
        type,
        time);

    // Create an instance of the assembly and insert it into the scene.
    asf::auto_release_ptr<asr::AssemblyInstance> assembly_instance(
        asr::AssemblyInstanceFactory::create(
            "assembly_inst",
            asr::ParamArray(),
            "assembly"));
    assembly_instance
        ->transform_sequence()
            .set_transform(0.0, asf::Transformd::identity());
    scene->assembly_instances().insert(assembly_instance);

    // Insert the assembly into the scene.
    scene->assemblies().insert(assembly);

    // Setup the environment.
    setup_environment(
        scene.ref(),
        rend_params,
        frame_rend_params,
        settings,
        time);

    // Create a camera and bind it to the scene.
    scene->set_camera(build_camera(view_params, bitmap, time));

    // Create a frame and bind it to the project.
    project->set_frame(
        build_frame(
            *scene->get_camera(),
            bitmap,
            rend_params));

    // Bind the scene to the project.
    project->set_scene(scene);

    // Apply renderer settings.
    settings.apply(project.ref());

    return project;
}
