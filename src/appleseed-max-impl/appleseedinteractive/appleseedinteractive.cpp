
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

// Interface header.
#include "appleseedinteractive.h"

// appleseed-max headers.
#include "appleseedinteractive/interactiverenderercontroller.h"
#include "appleseedinteractive/interactivesession.h"
#include "appleseedinteractive/interactivetilecallback.h"
#include "appleseedrenderer/appleseedrenderer.h"
#include "appleseedrenderer/projectbuilder.h"
#include "utilities.h"

// 3ds Max headers.
#include <assert1.h>
#include <matrix3.h>

// Standard headers.
#include <clocale>

namespace asf = foundation;
namespace asr = renderer;

namespace
{
    void get_view_params_from_viewport(
        ViewParams&             view_params,
        ViewExp&                view_exp,
        const TimeValue         time)
    {
        if (view_exp.IsAlive())
        {
            Matrix3 v_mat;
            ViewExp13* vp13 = reinterpret_cast<ViewExp13*>(view_exp.Execute(ViewExp::kEXECUTE_GET_VIEWEXP_13));
            vp13->GetAffineTM(v_mat);
            view_params.affineTM = v_mat;
            view_params.prevAffineTM = view_params.affineTM;
            view_params.projType = vp13->IsPerspView() ? PROJ_PERSPECTIVE : PROJ_PARALLEL;
            view_params.zoom = vp13->GetZoom();
            view_params.fov = vp13->GetFOV();
            view_params.distance = vp13->GetFocalDist();
            view_params.hither = vp13->GetHither();
            view_params.yon = vp13->GetYon();
        }
    }

    void get_view_params_from_view_node(
        ViewParams&             view_params,
        INode*                  view_node,
        const TimeValue         time)
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

                view_params.affineTM = Inverse(cam_to_world);

                CameraState cam_state;
                cam->EvalCameraState(time, validity_interval, &cam_state);

                view_params.projType = PROJ_PERSPECTIVE;
                view_params.fov = cam_state.fov;

                if (cam_state.manualClip)
                {
                    view_params.hither = cam_state.hither;
                    view_params.yon = cam_state.yon;
                }
                else
                {
                    view_params.hither = 0.001f;
                    view_params.yon = 1.0e38f;
                }
            }
            break;

          case LIGHT_CLASS_ID:
            DbgAssert(!"Not implemented yet.");
            break;

          default:
            DbgAssert(!"Unexpected super class ID for camera.");
        }
    }

    class RenderBeginProc
      : public RefEnumProc
    {
      public:
        explicit RenderBeginProc(const TimeValue time)
          : m_time(time)
        {
        }

        virtual int proc(ReferenceMaker* rm) override
        {
            rm->RenderBegin(m_time);
            return REF_ENUM_CONTINUE;
        }

      private:
        const TimeValue m_time;
    };

    class RenderEndProc
      : public RefEnumProc
    {
      public:
        explicit RenderEndProc(const TimeValue time)
          : m_time(time)
        {
        }

        virtual int proc(ReferenceMaker* rm) override
        {
            rm->RenderEnd(m_time);
            return REF_ENUM_CONTINUE;
        }

      private:
        const TimeValue m_time;
    };

    void render_begin(
        std::vector<INode*>&    nodes,
        const TimeValue         time)
    {
        RenderBeginProc proc(time);
        proc.BeginEnumeration();

        for (auto node : nodes)
            node->EnumRefHierarchy(proc);

        proc.EndEnumeration();
    }

    void render_end(
        std::vector<INode*>&    nodes,
        const TimeValue         time)
    {
        RenderEndProc proc(time);
        proc.BeginEnumeration();

        for (auto node : nodes)
            node->EnumRefHierarchy(proc);

        proc.EndEnumeration();
    }

    class SceneChangeCallback
      : public INodeEventCallback
    {
      public:
        SceneChangeCallback(AppleseedInteractiveRender* renderer, INode* camera)
          : m_renderer(renderer)
          , m_active_camera(camera)
        {
        }

        virtual void ControllerOtherEvent(NodeKeyTab& nodes) override
        {
            if (m_active_camera == nullptr || m_renderer == nullptr)
                return;
            
            for (int i = 0, e = nodes.Count(); i < e; ++i)
            {
                if (NodeEventNamespace::GetNodeByKey(nodes[i]) == m_active_camera)
                {
                    m_renderer->update_camera(m_active_camera);
                    m_renderer->get_render_session()->reininitialize_render();
                    break;
                }
            }
        }

      private:
        AppleseedInteractiveRender*     m_renderer;
        INode*                          m_active_camera;
    };
}


//
// AppleseedInteractiveRender class implementation.
//

AppleseedInteractiveRender::AppleseedInteractiveRender()
  : m_owner_wnd(0)
  , m_bitmap(nullptr)
  , m_iirender_mgr(nullptr)
  , m_scene_inode(nullptr)
  , m_use_view_inode(false)
  , m_view_inode(nullptr)
  , m_view_exp(nullptr)
  , m_progress_cb(nullptr)
{
    m_entities.clear();
}

AppleseedInteractiveRender::~AppleseedInteractiveRender()
{
    // Make sure the ActiveShade session has stopped.
    EndSession();
}

asf::auto_release_ptr<asr::Project> AppleseedInteractiveRender::prepare_project(
    const RendererSettings&     renderer_settings,
    const ViewParams&           view_params,
    const TimeValue             time)
{
    std::string previous_locale(std::setlocale(LC_ALL, "C"));

    RendParams rend_params;
    rend_params.inMtlEdit = false;
    rend_params.rendType = RENDTYPE_NORMAL;
    rend_params.envMap = GetCOREInterface()->GetUseEnvironmentMap() ? GetCOREInterface()->GetEnvironmentMap() : nullptr;

    FrameRendParams frame_rend_params;
    frame_rend_params.background = Color(GetCOREInterface()->GetBackGround(time, FOREVER));
    frame_rend_params.regxmin = frame_rend_params.regymin = 0;
    frame_rend_params.regxmax = frame_rend_params.regymax = 1;
    
    // Collect the entities we're interested in.
    if (m_progress_cb)
        m_progress_cb->SetTitle(L"Collecting Entities...");

    m_entities.clear();

    MaxSceneEntityCollector collector(m_entities);
    collector.collect(m_scene_inode);

    // Call RenderBegin() on all object instances.
    render_begin(m_entities.m_objects, time);

    // Build the project.
    if (m_progress_cb)
        m_progress_cb->SetTitle(L"Building Project...");

    asf::auto_release_ptr<asr::Project> project(
        build_project(
            m_entities,
            m_default_lights,
            m_view_inode,
            view_params,
            rend_params,
            frame_rend_params,
            renderer_settings,
            m_bitmap,
            time));

    std::setlocale(LC_ALL, previous_locale.c_str());

    return project;
}

void AppleseedInteractiveRender::update_camera(INode* camera)
{
    ViewParams view_params;
    get_view_params_from_view_node(view_params, camera, m_time);

    m_project->get_scene()->get_active_camera()->transform_sequence().set_transform(
        static_cast<float>(m_time),
        asf::Transformd::from_local_to_parent(to_matrix4d(Inverse(view_params.affineTM))));
}

InteractiveSession* AppleseedInteractiveRender::get_render_session()
{
    return m_render_session.get();
}

void AppleseedInteractiveRender::BeginSession()
{
    DbgAssert(m_render_session == nullptr);
    
    m_time = GetCOREInterface()->GetTime();

    ViewExp13* vp13 = reinterpret_cast<ViewExp13*>(GetViewExp()->Execute(ViewExp::kEXECUTE_GET_VIEWEXP_13));
    INode* active_cam = vp13->GetViewCamera();

    ViewParams view_params;
    if (GetUseViewINode())
        get_view_params_from_view_node(view_params, GetViewINode(), m_time);
    else
        get_view_params_from_viewport(view_params, *GetViewExp(), m_time);

    RendererSettings renderer_settings = RendererSettings::defaults();
    
    Renderer* curr_renderer = GetCOREInterface()->GetCurrentRenderer(false);
    if (curr_renderer && curr_renderer->ClassID() == AppleseedRenderer::get_class_id())
    {
        AppleseedRenderer* appleseed_renderer = static_cast<AppleseedRenderer*>(curr_renderer);
        renderer_settings = appleseed_renderer->get_renderer_settings();
        renderer_settings.m_output_mode = RendererSettings::OutputMode::RenderOnly;
    }
    
    m_project = prepare_project(renderer_settings, view_params, m_time);

    m_render_session.reset(new InteractiveSession(
        m_iirender_mgr,
        m_project.get(),
        renderer_settings,
        m_bitmap));

    if (m_progress_cb)
        m_progress_cb->SetTitle(L"Rendering...");

    m_node_callback.reset(new SceneChangeCallback(this, active_cam));
    m_callback_key = GetISceneEventManager()->RegisterCallback(m_node_callback.get(), false, 100, true);

    m_render_session->start_render();
}

void AppleseedInteractiveRender::EndSession()
{
    if (m_render_session != nullptr)
    {
        GetISceneEventManager()->UnRegisterCallback(m_callback_key);
        m_node_callback.reset(nullptr);

        m_render_session->abort_render();

        // Drain UI message queue to process bitmap updates posted from tilecallback.
        MSG msg;
        while (PeekMessage(&msg, GetCOREInterface()->GetMAXHWnd(), 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        m_render_session->end_render();

        m_render_session.reset(nullptr);
    }
    
    render_end(m_entities.m_objects, m_time);

    if (m_progress_cb)
        m_progress_cb->SetTitle(L"Done.");
}

void AppleseedInteractiveRender::SetOwnerWnd(HWND owner_wnd)
{
    m_owner_wnd = owner_wnd;
}

HWND AppleseedInteractiveRender::GetOwnerWnd() const
{
    return m_owner_wnd;
}

void AppleseedInteractiveRender::SetIIRenderMgr(IIRenderMgr* iirender_mgr)
{
    m_iirender_mgr = iirender_mgr;
}

IIRenderMgr* AppleseedInteractiveRender::GetIIRenderMgr(IIRenderMgr* iirender_mgr) const
{
    return m_iirender_mgr;
}

void AppleseedInteractiveRender::SetBitmap(Bitmap* bitmap)
{
    m_bitmap = bitmap;
}

Bitmap* AppleseedInteractiveRender::GetBitmap(Bitmap* bitmap) const
{
    return m_bitmap;
}

void AppleseedInteractiveRender::SetSceneINode(INode* scene_inode)
{
    m_scene_inode = scene_inode;
}

INode* AppleseedInteractiveRender::GetSceneINode() const
{
    return m_scene_inode;
}

void AppleseedInteractiveRender::SetUseViewINode(bool use_view_inode)
{
    m_use_view_inode = use_view_inode;
}

bool AppleseedInteractiveRender::GetUseViewINode() const
{
    return m_use_view_inode;
}

void AppleseedInteractiveRender::SetViewINode(INode* view_inode)
{
    m_view_inode = view_inode;
}

INode* AppleseedInteractiveRender::GetViewINode() const
{
    return m_view_inode;
}

void AppleseedInteractiveRender::SetViewExp(ViewExp* view_exp)
{
    m_view_exp = view_exp;
}

ViewExp* AppleseedInteractiveRender::GetViewExp() const
{
    return m_view_exp;
}

void AppleseedInteractiveRender::SetRegion(const Box2& region)
{
    m_region = region;
}

const Box2& AppleseedInteractiveRender::GetRegion() const
{
    return m_region;
}

void AppleseedInteractiveRender::SetDefaultLights(DefaultLight* def_lights, int num_def_lights)
{
    m_default_lights.clear();

    if (def_lights != nullptr)
        m_default_lights.insert(m_default_lights.begin(), def_lights, def_lights + num_def_lights);
}

const DefaultLight* AppleseedInteractiveRender::GetDefaultLights(int& num_def_lights) const
{
    num_def_lights = static_cast<int>(m_default_lights.size());
    return m_default_lights.data();
}

void AppleseedInteractiveRender::SetProgressCallback(IRenderProgressCallback* prog_cb)
{
    m_progress_cb = prog_cb;
}

const IRenderProgressCallback* AppleseedInteractiveRender::GetProgressCallback() const
{
    return m_progress_cb;
}

void AppleseedInteractiveRender::Render(Bitmap* bitmap)
{
}

ULONG AppleseedInteractiveRender::GetNodeHandle(int x, int y)
{
    return 0;
}

bool AppleseedInteractiveRender::GetScreenBBox(Box2& s_bbox, INode* inode)
{
    return FALSE;
}

ActionTableId AppleseedInteractiveRender::GetActionTableId()
{
    return 0;
}

ActionCallback* AppleseedInteractiveRender::GetActionCallback()
{
    return nullptr;
}

BOOL AppleseedInteractiveRender::IsRendering()
{
    return m_render_session != nullptr;
}

#if MAX_RELEASE > MAX_RELEASE_R17
void AppleseedInteractiveRender::AbortRender()
{
    EndSession();
}
#endif
