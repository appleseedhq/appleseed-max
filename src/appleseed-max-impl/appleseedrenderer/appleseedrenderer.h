
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

// appleseed-max headers.
#include "appleseedrenderer/maxsceneentities.h"
#include "appleseedrenderer/renderersettings.h"

// Build options header.
#include "renderer/api/buildoptions.h"

// appleseed.foundation headers.
#include "foundation/platform/windows.h"    // include before 3ds Max headers

// 3ds Max headers.
#include <iparamb2.h>
#include <ITabDialog.h>
#include <max.h>
#include <render.h>
#undef base_type

// Standard headers.
#include <vector>

// Windows headers.
#include <tchar.h>

// Forward declarations.
class AppleseedInteractiveRender;

class AppleseedRendererPBlockAccessor
  : public PBAccessor
{
  public:
    void Get(
        PB2Value&       v,
        ReferenceMaker* owner,
        ParamID         id,
        int             tab_index,
        TimeValue       t,
        Interval        &valid) override;

    void Set(
        PB2Value&       v,
        ReferenceMaker* owner,
        ParamID         id,
        int             tab_index,
        TimeValue       t) override;
};

class AppleseedRenderer
  : public Renderer
  , public ITabDialogObject
#if MAX_RELEASE < 19000
  , public IRendererRequirements
#endif
{
  public:
    static Class_ID get_class_id();

    AppleseedRenderer();

    const RendererSettings& get_renderer_settings();

    // ReferenceMaker methods.
    int NumRefs() override;
    RefTargetHandle GetReference(int i) override;
    void SetReference(int i, RefTargetHandle rtarg) override;
    Class_ID ClassID() override;

    void GetClassName(MSTR& s) override;

    void DeleteThis() override;
    int NumParamBlocks() override;
    IParamBlock2* GetParamBlock(int i) override;
    IParamBlock2* GetParamBlockByID(BlockID id) override;

    // Animatable.
    void* GetInterface(ULONG id) override;
    BaseInterface* GetInterface(Interface_ID id) override;

    bool HasRequirement(Requirement requirement) override;

#if MAX_RELEASE > 18000

    bool IsStopSupported() const override;
    void StopRendering() override;

    PauseSupport IsPauseSupported() const override;
    void PauseRendering() override;
    void ResumeRendering() override;

    bool CompatibleWithAnyRenderElement() const override;
    bool CompatibleWithRenderElement(IRenderElement& pIRenderElement) const override;

    IInteractiveRender* GetIInteractiveRender() override;

    void GetVendorInformation(MSTR& info) const override;
    void GetPlatformInformation(MSTR& info) const override;

#endif

    RefTargetHandle Clone(RemapDir& remap) override;

    RefResult NotifyRefChanged(
        const Interval&         changeInt,
        RefTargetHandle         hTarget,
        PartID&                 partID,
        RefMessage              message,
        BOOL                    propagate) override;

    int Open(
        INode*                  scene,
        INode*                  view_node,
        ViewParams*             view_params,
        RendParams&             rend_params,
        HWND                    hwnd,
        DefaultLight*           default_lights,
        int                     default_light_count,
        RendProgressCallback*   progress_cb) override;

    int Render(
        TimeValue               time,
        Bitmap*                 bitmap,
        FrameRendParams&        frame_rend_params,
        HWND                    hwnd,
        RendProgressCallback*   progress_cb,
        ViewParams*             view_params) override;

    void Close(
        HWND                    hwnd,
        RendProgressCallback*   progress_cb) override;

    RendParamDlg* CreateParamDialog(
        IRendParams*            rend_params,
        BOOL                    in_progress) override;

    void ResetParams() override;

    IOResult Save(ISave* isave) override;
    IOResult Load(ILoad* iload) override;

    // ITabDialog.
    void AddTabToDialog(
        ITabbedDialog*          dialog,
        ITabDialogPluginTab*    tab) override;
    int AcceptTab(
        ITabDialogPluginTab*    tab) override;

    void show_last_session_log();
    void create_log_window();

  private:
    friend AppleseedRendererPBlockAccessor;

    AppleseedInteractiveRender* m_interactive_renderer;
    RendererSettings            m_settings;
    INode*                      m_scene;
    INode*                      m_view_node;
    ViewParams                  m_view_params;
    RendParams                  m_rend_params;
    std::vector<DefaultLight>   m_default_lights;
    TimeValue                   m_time;
    MaxSceneEntities            m_entities;
    IParamBlock2*               m_param_block;

    void clear();
};


//
// AppleseedRenderer class descriptor.
//

class AppleseedRendererClassDesc
  : public ClassDesc2
{
  public:
    int IsPublic() override;
    void* Create(BOOL loading) override;
    const MCHAR* ClassName() override;
    SClass_ID SuperClassID() override;
    Class_ID ClassID() override;
    const MCHAR* Category() override;
    const MCHAR* InternalName() override;
    const MCHAR* GetRsrcString(INT_PTR id) override;
};

extern AppleseedRendererClassDesc g_appleseed_renderer_classdesc;
