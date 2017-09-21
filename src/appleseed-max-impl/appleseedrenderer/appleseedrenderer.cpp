
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
#include "appleseedrenderer.h"

// appleseed-max headers.
#include "appleseedinteractive/appleseedinteractive.h"
#include "appleseedrenderer/appleseedrendererparamdlg.h"
#include "appleseedrenderer/datachunks.h"
#include "appleseedrenderer/projectbuilder.h"
#include "appleseedrenderer/renderercontroller.h"
#include "appleseedrenderer/tilecallback.h"
#include "utilities.h"
#include "version.h"

// appleseed.renderer headers.
#include "renderer/api/frame.h"
#include "renderer/api/project.h"
#include "renderer/api/rendering.h"

// appleseed.foundation headers.
#include "foundation/image/canvasproperties.h"
#include "foundation/image/image.h"
#include "foundation/platform/thread.h"
#include "foundation/platform/types.h"
#include "foundation/utility/autoreleaseptr.h"

// 3ds Max headers.
#include <assert1.h>
#include <bitmap.h>
#include <iimageviewer.h>
#include <interactiverender.h>

// Standard headers.
#include <clocale>
#include <cstddef>
#include <string>

namespace asf = foundation;
namespace asr = renderer;

namespace
{
    const Class_ID AppleseedRendererClassId(0x72651b24, 0x5da32e1d);
    const Class_ID AppleseedRendererTabClassId(0x6155c0c, 0xed6475b);
    const wchar_t* AppleseedRendererClassName = L"appleseed Renderer";
}

AppleseedRendererClassDesc g_appleseed_renderer_classdesc;


//
// AppleseedRenderer class implementation.
//

Class_ID AppleseedRenderer::get_class_id()
{
    return AppleseedRendererClassId;
}

AppleseedRenderer::AppleseedRenderer()
  : m_settings(RendererSettings::defaults())
  , m_interactive_renderer(nullptr)
{
    clear();
}

RendererSettings AppleseedRenderer::get_renderer_settings()
{
    return m_settings;
}

Class_ID AppleseedRenderer::ClassID()
{
    return AppleseedRendererClassId;
}

void AppleseedRenderer::GetClassName(MSTR& s)
{
    s = AppleseedRendererClassName;
}

void AppleseedRenderer::DeleteThis()
{
    delete m_interactive_renderer;
    delete this;
}

void* AppleseedRenderer::GetInterface(ULONG id)
{
#if MAX_RELEASE == MAX_RELEASE_R18  // No interactive render for max 2015.
    if (id == I_RENDER_ID)
    {
        if (m_interactive_renderer == nullptr)
        {
            m_interactive_renderer = new AppleseedInteractiveRender();
        }

        return static_cast<IInteractiveRender*>(m_interactive_renderer);
    }
    else
#endif
    {
        return Renderer::GetInterface(id);
    }
}

BaseInterface* AppleseedRenderer::GetInterface(Interface_ID id)
{
    if (id == TAB_DIALOG_OBJECT_INTERFACE_ID) {
        return static_cast<ITabDialogObject*>(this);
    }
    else
    {
        return Renderer::GetInterface(id);
    }
}

#if MAX_RELEASE == MAX_RELEASE_R19

bool AppleseedRenderer::IsStopSupported() const
{
    return false;
}

void AppleseedRenderer::StopRendering()
{
}

Renderer::PauseSupport AppleseedRenderer::IsPauseSupported() const
{
    return PauseSupport::None;
}

void AppleseedRenderer::PauseRendering()
{
}

void AppleseedRenderer::ResumeRendering()
{
}

bool AppleseedRenderer::HasRequirement(Requirement requirement)
{
    // todo: specify kRequirement_NoVFB and kRequirement_DontSaveRenderOutput when saving project instead of rendering.

    switch (requirement)
    {
      case kRequirement_Wants32bitFPOutput: return true;
      case kRequirement_SupportsConcurrentRendering: return true;
    }

    return false;
}

bool AppleseedRenderer::CompatibleWithAnyRenderElement() const
{
    return false;
}

bool AppleseedRenderer::CompatibleWithRenderElement(IRenderElement& pIRenderElement) const
{
    return false;
}

IInteractiveRender* AppleseedRenderer::GetIInteractiveRender()
{
    if (m_interactive_renderer == nullptr)
        m_interactive_renderer = new AppleseedInteractiveRender();

    return static_cast<IInteractiveRender*>(m_interactive_renderer);
}

void AppleseedRenderer::GetVendorInformation(MSTR& info) const
{
    info = L"appleseed-max ";
    info += PluginVersionString;
}

void AppleseedRenderer::GetPlatformInformation(MSTR& info) const
{
}

#endif

RefTargetHandle AppleseedRenderer::Clone(RemapDir& remap)
{
    AppleseedRenderer* new_rend = static_cast<AppleseedRenderer*>(g_appleseed_renderer_classdesc.Create(false));
    
    DbgAssert(new_rend != nullptr);
    
    for (int i = 0, e = NumRefs(); i < e; ++i)
        new_rend->ReplaceReference(i, remap.CloneRef(GetReference(i)));
        
    BaseClone(this, new_rend, remap);

    return new_rend;
}

RefResult AppleseedRenderer::NotifyRefChanged(
    const Interval&         changeInt,
    RefTargetHandle         hTarget,
    PartID&                 partID,
    RefMessage              message,
    BOOL                    propagate)
{
    return REF_SUCCEED;
}

int AppleseedRenderer::Open(
    INode*                  scene,
    INode*                  view_node,
    ViewParams*             view_params,
    RendParams&             rend_params,
    HWND                    hwnd,
    DefaultLight*           default_lights,
    int                     default_light_count,
    RendProgressCallback*   progress_cb)
{
    SuspendAll suspend(TRUE, TRUE, TRUE, TRUE, TRUE, TRUE);

    m_scene = scene;
    m_view_node = view_node;
    if (view_params)
        m_view_params = *view_params;
    m_rend_params = rend_params;

    // Copy the default lights as the 'default_lights' pointer is no longer valid in Render().
    m_default_lights.clear();
    m_default_lights.reserve(default_light_count);
    for (int i = 0; i < default_light_count; ++i)
        m_default_lights.push_back(default_lights[i]);

    return 1;   // success
}

namespace
{
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
            {
                DbgAssert(!"Not implemented yet.");
            }
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

    void render(
        asr::Project&           project,
        const RendererSettings& settings,
        Bitmap*                 bitmap,
        RendProgressCallback*   progress_cb)
    {
        // Number of rendered tiles, shared counter accessed atomically.
        volatile asf::uint32 rendered_tile_count = 0;

        // Create the renderer controller.
        const size_t total_tile_count =
              static_cast<size_t>(settings.m_passes)
            * project.get_frame()->image().properties().m_tile_count;
        RendererController renderer_controller(
            progress_cb,
            &rendered_tile_count,
            total_tile_count);

        // Create the tile callback.
        TileCallback tile_callback(bitmap, &rendered_tile_count);

        // Create the master renderer.
        std::auto_ptr<asr::MasterRenderer> renderer(
            new asr::MasterRenderer(
                project,
                project.configurations().get_by_name("final")->get_inherited_parameters(),
                &renderer_controller,
                &tile_callback));

        // Render the frame.
        renderer->render();

        // Make sure the master renderer is deleted before the project.
    }
}

int AppleseedRenderer::Render(
    TimeValue               time,
    Bitmap*                 bitmap,
    FrameRendParams&        frame_rend_params,
    HWND                    hwnd,
    RendProgressCallback*   progress_cb,
    ViewParams*             view_params)
{
    SuspendAll suspend(TRUE, TRUE, TRUE, TRUE, TRUE, TRUE);

    std::string previous_locale(std::setlocale(LC_ALL, "C"));

    m_time = time;

    if (view_params)
        m_view_params = *view_params;

    if (m_view_node)
        get_view_params_from_view_node(m_view_params, m_view_node, time);

    // Retrieve and tweak renderer settings.
    RendererSettings renderer_settings = m_settings;
    if (m_rend_params.inMtlEdit)
    {
        renderer_settings.m_pixel_samples = m_rend_params.mtlEditAA ? 32 : 4;
        renderer_settings.m_passes = 1;
        renderer_settings.m_gi = true;
        renderer_settings.m_background_emits_light = false;
    }

    // Collect the entities we're interested in.
    if (progress_cb)
        progress_cb->SetTitle(L"Collecting Entities...");
    m_entities.clear();
    MaxSceneEntityCollector collector(m_entities);
    collector.collect(m_scene);

    // Call RenderBegin() on all object instances.
    render_begin(m_entities.m_objects, m_time);

    // Build the project.
    if (progress_cb)
        progress_cb->SetTitle(L"Building Project...");
    asf::auto_release_ptr<asr::Project> project(
        build_project(
            m_entities,
            m_default_lights,
            m_view_node,
            m_view_params,
            m_rend_params,
            frame_rend_params,
            renderer_settings,
            bitmap,
            time));

    if (m_rend_params.inMtlEdit)
    {
        // Write the project to disk, useful to debug material previews.
        // asr::ProjectFileWriter::write(project.ref(), "MaterialEditor.appleseed");

        // Render the project.
        if (progress_cb)
            progress_cb->SetTitle(L"Rendering...");
        render(project.ref(), m_settings, bitmap, progress_cb);
    }
    else
    {
        // Write the project to disk.
        if (!m_settings.m_use_max_procedural_maps)
        {
            if (m_settings.m_output_mode == RendererSettings::OutputMode::SaveProjectOnly ||
                m_settings.m_output_mode == RendererSettings::OutputMode::SaveProjectAndRender)
            {
                if (progress_cb)
                    progress_cb->SetTitle(L"Writing Project To Disk...");
                asr::ProjectFileWriter::write(
                    project.ref(),
                    wide_to_utf8(m_settings.m_project_file_path).c_str());
            }
        }

        // Render the project.
        if (m_settings.m_output_mode == RendererSettings::OutputMode::RenderOnly ||
            m_settings.m_output_mode == RendererSettings::OutputMode::SaveProjectAndRender)
        {
            if (progress_cb)
                progress_cb->SetTitle(L"Rendering...");
            if (m_settings.m_low_priority_mode)
            {
                asf::ProcessPriorityContext background_context(
                    asf::ProcessPriority::ProcessPriorityLow,
                    &asr::global_logger());
                render(project.ref(), m_settings, bitmap, progress_cb);
            }
            else
            {
                render(project.ref(), m_settings, bitmap, progress_cb);
            }
        }
    }

    if (progress_cb)
        progress_cb->SetTitle(L"Done.");

    std::setlocale(LC_ALL, previous_locale.c_str());

    // Success.
    return 1;
}

void AppleseedRenderer::Close(
    HWND                    hwnd,
    RendProgressCallback*   progress_cb)
{
    // Call RenderEnd() on all object instances.
    render_end(m_entities.m_objects, m_time);

    clear();
}

RendParamDlg* AppleseedRenderer::CreateParamDialog(
    IRendParams*            rend_params,
    BOOL                    in_progress)
{
    return
        new AppleseedRendererParamDlg(
            rend_params,
            in_progress,
            m_settings);
}

void AppleseedRenderer::ResetParams()
{
    clear();
}

IOResult AppleseedRenderer::Save(ISave* isave)
{
    bool success = true;

    isave->BeginChunk(ChunkFileFormatVersion);
    success &= write(isave, FileFormatVersion);
    isave->EndChunk();

    isave->BeginChunk(ChunkSettings);
    success &= m_settings.save(isave);
    isave->EndChunk();

    return success ? IO_OK : IO_ERROR;
}

IOResult AppleseedRenderer::Load(ILoad* iload)
{
    IOResult result = IO_OK;

    while (true)
    {
        result = iload->OpenChunk();
        if (result == IO_END)
            return IO_OK;
        if (result != IO_OK)
            break;

        switch (iload->CurChunkID())
        {
          case ChunkFileFormatVersion:
            {
                USHORT version;
                result = read<USHORT>(iload, &version);
            }
            break;

          case ChunkSettings:
            result = m_settings.load(iload);
            break;
        }

        if (result != IO_OK)
            break;

        result = iload->CloseChunk();
        if (result != IO_OK)
            break;
    }

    return result;
}

void AppleseedRenderer::clear()
{
    m_scene = nullptr;
    m_view_node = nullptr;
    m_default_lights.clear();
    m_time = 0;
    m_entities.clear();
}

void AppleseedRenderer::AddTabToDialog(
    ITabbedDialog*          dialog,
    ITabDialogPluginTab*    tab) 
{
    const int rend_rollup_width = 222;  // The width of the render rollout in dialog units.
    dialog->AddRollout(
        L"Renderer",
        NULL,
        AppleseedRendererTabClassId,
        tab,
        -1,
        rend_rollup_width,
        0,
        0,
        ITabbedDialog::kSystemPage);
}

int AppleseedRenderer::AcceptTab(
    ITabDialogPluginTab*    tab)
{
    const Class_ID raytrace_tab_classid(0x4fa95e9b, 0x9a26e66);

    if (tab->GetSuperClassID() == RADIOSITY_CLASS_ID)
        return 0;
    
    if (tab->GetClassID() == raytrace_tab_classid)
        return 0;

    return TAB_DIALOG_ADD_TAB;
}


//
// AppleseedRendererClassDesc class implementation.
//

int AppleseedRendererClassDesc::IsPublic()
{
    return TRUE;
}

void* AppleseedRendererClassDesc::Create(BOOL loading)
{
    return new AppleseedRenderer();
}

const MCHAR* AppleseedRendererClassDesc::ClassName()
{
    return AppleseedRendererClassName;
}

SClass_ID AppleseedRendererClassDesc::SuperClassID()
{
    return RENDERER_CLASS_ID;
}

Class_ID AppleseedRendererClassDesc::ClassID()
{
    return AppleseedRendererClassId;
}

const MCHAR* AppleseedRendererClassDesc::Category()
{
    return L"";
}

const MCHAR* AppleseedRendererClassDesc::InternalName()
{
    // Parsable name used by MAXScript.
    return L"appleseedRenderer";
}
