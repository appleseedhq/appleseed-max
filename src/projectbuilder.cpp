
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2015 Francois Beaune, The appleseedhq Organization
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
#include "maxsceneentities.h"
#include "utilities.h"

// appleseed.renderer headers.
#include "renderer/api/camera.h"
#include "renderer/api/environment.h"
#include "renderer/api/frame.h"
#include "renderer/api/object.h"
#include "renderer/api/project.h"
#include "renderer/api/scene.h"
#include "renderer/api/utility.h"

// appleseed.foundation headers.
#include "foundation/math/scalar.h"
#include "foundation/math/transform.h"
#include "foundation/platform/compiler.h"
#include "foundation/utility/containers/dictionary.h"

// Boost headers.
#include "boost/static_assert.hpp"

// 3ds Max headers.
#include <bitmap.h>
#include <object.h>
#include <render.h>
#include <triobj.h>

// Standard headers.
#include <cassert>
#include <cstddef>
#include <limits>
#include <string>
#include <vector>

namespace asf = foundation;
namespace asr = renderer;

namespace
{
    asf::auto_release_ptr<asr::Camera> build_camera(
        INode*                  view_node,
        const ViewParams&       view_params,
        Bitmap*                 bitmap,
        const TimeValue         time)
    {
        asr::ParamArray params;

        if (view_params.projType == PROJ_PERSPECTIVE)
        {
            params.insert("film_dimensions", make_vec_string(bitmap->Width(), bitmap->Height()));
            params.insert("horizontal_fov", asf::rad_to_deg(view_params.fov));
        }
        else
        {
            assert(view_params.projType == PROJ_PARALLEL);

            const float ViewDefaultWidth = 400.0f;
            const float aspect = static_cast<float>(bitmap->Height()) / bitmap->Width();
            const float film_width = ViewDefaultWidth * view_params.zoom;
            const float film_height = film_width * aspect;
            params.insert("film_dimensions", make_vec_string(film_width, film_height));
        }

        asf::Matrix4d camera_matrix = max_to_as(Inverse(view_params.affineTM));

        if (view_node)
        {
            const ObjectState& os = view_node->EvalWorldState(time);
            switch (os.obj->SuperClassID())
            {
              case CAMERA_CLASS_ID:
                {
                    CameraObject* cam = static_cast<CameraObject*>(os.obj);

                    Interval validity_interval;
                    validity_interval.SetInfinite();

                    Matrix3 cam_to_world = view_node->GetObjTMAfterWSM(time, &validity_interval);
                    cam_to_world.NoScale();
                    camera_matrix = max_to_as(cam_to_world);

                    CameraState cam_state;
                    cam->EvalCameraState(time, validity_interval, &cam_state);

                    if (cam_state.manualClip)
                        params.insert("near_z", cam_state.hither);
                }
                break;

              case LIGHT_CLASS_ID:
                {
                    // todo: implement.
                }
                break;

              default:
                assert(!"Unexpected super class ID  for camera.");
            }
        }

        asf::auto_release_ptr<renderer::Camera> camera =
            view_params.projType == PROJ_PERSPECTIVE
                ? asr::PinholeCameraFactory().create("camera", params)
                : asr::OrthographicCameraFactory().create("camera", params);

        camera->transform_sequence().set_transform(
            0.0, asf::Transformd::from_local_to_parent(camera_matrix));

        return camera;
    }

    TriObject* get_tri_object_from_node(
        const ObjectState&      object_state,
        const TimeValue         time,
        bool&                   must_delete)
    {
        assert(object_state.obj);

        must_delete = false;

        const Class_ID TriObjectClassID(TRIOBJ_CLASS_ID, 0);

        if (!object_state.obj->CanConvertToType(TriObjectClassID))
            return 0;

        TriObject* tri_object = static_cast<TriObject*>(object_state.obj->ConvertToType(time, TriObjectClassID));
        must_delete = tri_object != object_state.obj;

        return tri_object;
    }

    void add_object(
        asr::Assembly&          assembly,
        INode*                  node,
        const TimeValue         time)
    {
        // Retrieve the name of the referenced object.
        Object* max_object = node->GetObjectRef();
        std::string object_name = utf8_encode(max_object->GetObjectName());

        // todo: handle instancing.
        if (assembly.objects().get_by_name(object_name.c_str()) != 0)
            object_name = asr::make_unique_name(object_name, assembly.objects());

        // Create the object if it doesn't already exist in the appleseed scene.
        if (assembly.objects().get_by_name(object_name.c_str()) == 0)
        {
            // Retrieve the ObjectState at the desired time.
            const ObjectState object_state = node->EvalWorldState(time);

            // Convert the object to a TriObject.
            bool must_delete_tri_object;
            TriObject* tri_object = get_tri_object_from_node(object_state, time, must_delete_tri_object);
            if (tri_object == 0)
            {
                // todo: emit a message?
                return;
            }

            // Create a new mesh object.
            asf::auto_release_ptr<asr::MeshObject> object(
                asr::MeshObjectFactory::create(object_name.c_str(), asr::ParamArray()));
            {
                Mesh& mesh = tri_object->GetMesh();

                // Make sure the input mesh has vertex normals.
                mesh.checkNormals(TRUE);

                // Copy vertices to the mesh object.
                object->reserve_vertices(mesh.getNumVerts());
                for (int i = 0, e = mesh.getNumVerts(); i < e; ++i)
                {
                    const Point3& v = mesh.getVert(i);
                    object->push_vertex(asr::GVector3(v.x, v.y, v.z));
                }

                // Copy vertex normals and triangles to mesh object.
                object->reserve_vertex_normals(mesh.getNumFaces() * 3);
                object->reserve_triangles(mesh.getNumFaces());
                for (int i = 0, e = mesh.getNumFaces(); i < e; ++i)
                {
                    Face& face = mesh.faces[i];
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

                    BOOST_STATIC_ASSERT(sizeof(DWORD) == sizeof(asf::uint32));

                    asr::Triangle triangle;
                    triangle.m_v0 = face.getVert(0);
                    triangle.m_v1 = face.getVert(1);
                    triangle.m_v2 = face.getVert(2);
                    triangle.m_n0 = normal_indices[0];
                    triangle.m_n1 = normal_indices[1];
                    triangle.m_n2 = normal_indices[2];
                    triangle.m_a0 = asr::Triangle::None;
                    triangle.m_a1 = asr::Triangle::None;
                    triangle.m_a2 = asr::Triangle::None;

                    object->push_triangle(triangle);
                }

                // Delete the TriObject if necessary.
                if (must_delete_tri_object)
                    tri_object->DeleteMe();

                // todo: optimize the object.
            }

            // Insert the object into the assembly.
            assembly.objects().insert(asf::auto_release_ptr<asr::Object>(object));
        }

        // Figure out a unique name for this instance.
        std::string instance_name = utf8_encode(node->GetName());
        if (assembly.object_instances().get_by_name(instance_name.c_str()) != 0)
            instance_name = asr::make_unique_name(instance_name, assembly.object_instances());

        // Create an instance of the object and insert it into the assembly.
        const Matrix3 obj_to_world = node->GetObjTMAfterWSM(time);
        assembly.object_instances().insert(
            asr::ObjectInstanceFactory::create(
                instance_name.c_str(),
                asr::ParamArray(),
                object_name.c_str(),
                asf::Transformd::from_local_to_parent(max_to_as(obj_to_world)),
                asf::StringDictionary()));
    }

    void populate_assembly(
        asr::Assembly&          assembly,
        const MaxSceneEntities& entities,
        const TimeValue         time)
    {
        for (size_t i = 0, e = entities.m_instances.size(); i < e; ++i)
            add_object(assembly, entities.m_instances[i], time);
    }
}

asf::auto_release_ptr<asr::Project> build_project(
    const MaxSceneEntities&     entities,
    INode*                      view_node,
    const ViewParams&           view_params,
    Bitmap*                     bitmap,
    const TimeValue             time)
{
    // Create an empty project.
    asf::auto_release_ptr<asr::Project> project(
        asr::ProjectFactory::create("project"));

    // Add default configurations to the project.
    project->add_default_configurations();

    // Set the number of samples.
    project->configurations()
        .get_by_name("final")->get_parameters()
            .insert_path("uniform_pixel_renderer.samples", "1");

    // Create a scene.
    asf::auto_release_ptr<asr::Scene> scene(asr::SceneFactory::create());

    // Create an assembly.
    asf::auto_release_ptr<asr::Assembly> assembly(
        asr::AssemblyFactory().create("assembly", asr::ParamArray()));

    // Populate the assembly with entities from the 3ds Max scene.
    populate_assembly(assembly.ref(), entities, time);

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

    // Create a default environment and bind it to the scene.
    scene->set_environment(
        asr::EnvironmentFactory::create("environment", asr::ParamArray()));

    // Create a camera.
    scene->set_camera(
        build_camera(
            view_node,
            view_params,
            bitmap,
            time));

    // Create a frame and bind it to the project.
    project->set_frame(
        asr::FrameFactory::create(
            "beauty",
            asr::ParamArray()
                .insert("camera", scene->get_camera()->get_name())
                .insert("resolution", make_vec_string(bitmap->Width(), bitmap->Height()))
                .insert("color_space", "linear_rgb")));

    // Bind the scene to the project.
    project->set_scene(scene);

    return project;
}
