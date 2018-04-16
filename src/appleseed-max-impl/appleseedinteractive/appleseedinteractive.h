
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

#pragma once

// appleseed-max headers.
#include "appleseedrenderer/maxsceneentities.h"

// appleseed.foundation headers.
#include "foundation/platform/windows.h"    // include before 3ds Max headers
#include "foundation/utility/autoreleaseptr.h"

// 3ds Max headers.
#include <interactiverender.h>
#include <ISceneEventManager.h>
#include <maxapi.h>
#if MAX_RELEASE == MAX_RELEASE_R18
#include <Rendering/IAbortableRenderer.h>
#endif

// Standard headers.
#include <memory>
#include <vector>

// Forward declarations.
namespace renderer { class Project; }
class InteractiveSession;
class RendererSettings;
class ViewParams;

class AppleseedInteractiveRender
  : public IInteractiveRender
#if MAX_RELEASE == MAX_RELEASE_R18
  , public MaxSDK::IAbortableRenderer
#endif
{
  public:
    AppleseedInteractiveRender();
    virtual ~AppleseedInteractiveRender();

    // IInteractiveRender methods.
    virtual void BeginSession() override;
    virtual void EndSession() override;
    virtual void SetOwnerWnd(HWND owner_wnd) override;
    virtual HWND GetOwnerWnd() const override;
    virtual void SetIIRenderMgr(IIRenderMgr* iirender_mgr) override;
    virtual IIRenderMgr* GetIIRenderMgr(IIRenderMgr* iirender_mgr) const override;
    virtual void SetBitmap(Bitmap* bitmap) override;
    virtual Bitmap* GetBitmap(Bitmap* bitmap) const override;
    virtual void SetSceneINode(INode* scene_inode) override;
    virtual INode* GetSceneINode() const override;
    virtual void SetUseViewINode(bool use_view_inode) override;
    virtual bool GetUseViewINode() const override;
    virtual void SetViewINode(INode* view_inode) override;
    virtual INode* GetViewINode() const override;
    virtual void SetViewExp(ViewExp* view_exp) override;
    virtual ViewExp* GetViewExp() const override;
    virtual void SetRegion(const Box2& region) override;
    virtual const Box2& GetRegion() const override;
    virtual void SetDefaultLights(DefaultLight* def_lights, int num_def_lights) override;
    virtual const DefaultLight* GetDefaultLights(int& num_def_lights) const override;
    virtual void SetProgressCallback(IRenderProgressCallback* prog_cb) override;
    virtual const IRenderProgressCallback* GetProgressCallback() const override;
    virtual void Render(Bitmap* bitmap) override;
    virtual ULONG GetNodeHandle(int x, int y) override;
    virtual bool GetScreenBBox(Box2& s_bbox, INode* inode) override;
    virtual ActionTableId GetActionTableId() override;
    virtual ActionCallback* GetActionCallback() override;
    virtual BOOL IsRendering() override;

#if MAX_RELEASE > MAX_RELEASE_R17
    // IAbortableRenderer methods.
    virtual void AbortRender() override;
#endif

    void update_camera_object(INode* camera);
    void update_render_view();
    InteractiveSession* get_render_session();

  private:
    std::unique_ptr<InteractiveSession>             m_render_session;
    std::unique_ptr<INodeEventCallback>             m_node_callback;
    std::unique_ptr<RedrawViewsCallback>            m_view_callback;
    foundation::auto_release_ptr<renderer::Project> m_project;
    Bitmap*                                         m_bitmap;
    std::vector<DefaultLight>                       m_default_lights;
    IIRenderMgr*                                    m_iirender_mgr;
    HWND                                            m_owner_wnd;
    IRenderProgressCallback*                        m_progress_cb;
    MaxSceneEntities                                m_entities;
    TimeValue                                       m_time;
    Box2                                            m_region;
    INode*                                          m_scene_inode;
    ViewExp*                                        m_view_exp;
    INode*                                          m_view_inode;
    bool                                            m_use_view_inode;

    foundation::auto_release_ptr<renderer::Project> prepare_project(
        const RendererSettings&     renderer_settings,
        const ViewParams&           view_params,
        INode*                      camera_node,
        const TimeValue             time);
};
