
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

#pragma once

// appleseed-max headers.
#include "appleseedrenderer/maxsceneentities.h"
#include "appleseedrenderer/renderersettings.h"

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

class AppleseedRenderer
  : public Renderer
  , ITabDialogObject
{
  public:
    static Class_ID get_class_id();

    AppleseedRenderer();

    virtual Class_ID ClassID() override;

    virtual void GetClassName(MSTR& s) override;

    virtual void DeleteThis() override;

    // Animatable.
    virtual void* GetInterface(ULONG id) override;

    virtual BaseInterface* GetInterface(Interface_ID id) override;

#if MAX_RELEASE == MAX_RELEASE_R19

    virtual bool IsStopSupported() const override;
    virtual void StopRendering() override;

    virtual PauseSupport IsPauseSupported() const override;
    virtual void PauseRendering() override;
    virtual void ResumeRendering() override;

    virtual bool HasRequirement(Requirement requirement) override;

    virtual bool CompatibleWithAnyRenderElement() const override;
    virtual bool CompatibleWithRenderElement(IRenderElement& pIRenderElement) const override;

    virtual IInteractiveRender* GetIInteractiveRender() override;

    virtual void GetVendorInformation(MSTR& info) const override;
    virtual void GetPlatformInformation(MSTR& info) const override;

#endif

    virtual RefTargetHandle Clone(RemapDir& remap) override;

    virtual RefResult NotifyRefChanged(
        const Interval&         changeInt,
        RefTargetHandle         hTarget,
        PartID&                 partID,
        RefMessage              message,
        BOOL                    propagate) override;

    virtual int Open(
        INode*                  scene,
        INode*                  view_node,
        ViewParams*             view_params,
        RendParams&             rend_params,
        HWND                    hwnd,
        DefaultLight*           default_lights,
        int                     default_light_count,
        RendProgressCallback*   progress_cb) override;

    virtual int Render(
        TimeValue               time,
        Bitmap*                 bitmap,
        FrameRendParams&        frame_rend_params,
        HWND                    hwnd,
        RendProgressCallback*   progress_cb,
        ViewParams*             view_params) override;

    virtual void Close(
        HWND                    hwnd,
        RendProgressCallback*   progress_cb) override;

    virtual RendParamDlg* CreateParamDialog(
        IRendParams*            rend_params,
        BOOL                    in_progress) override;

    virtual void ResetParams() override;

    virtual IOResult Save(ISave* isave) override;
    virtual IOResult Load(ILoad* iload) override;

    // ITabDialog.
    virtual void AddTabToDialog(
        ITabbedDialog*          dialog,
        ITabDialogPluginTab*    tab) override;

    virtual int AcceptTab(
        ITabDialogPluginTab*    tab) override;

  private:
    AppleseedInteractiveRender* m_interactive_renderer;
    RendererSettings            m_settings;
    INode*                      m_scene;
    INode*                      m_view_node;
    ViewParams                  m_view_params;
    RendParams                  m_rend_params;
    std::vector<DefaultLight>   m_default_lights;
    TimeValue                   m_time;
    MaxSceneEntities            m_entities;

    void clear();
};


//
// AppleseedRenderer class descriptor.
//

class AppleseedRendererClassDesc
  : public ClassDesc2
{
  public:
    virtual int IsPublic() override;
    virtual void* Create(BOOL loading) override;
    virtual const MCHAR* ClassName() override;
    virtual SClass_ID SuperClassID() override;
    virtual Class_ID ClassID() override;
    virtual const MCHAR* Category() override;
    virtual const MCHAR* InternalName() override;
};

extern AppleseedRendererClassDesc g_appleseed_renderer_classdesc;
