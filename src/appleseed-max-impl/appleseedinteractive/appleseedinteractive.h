
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
#include "appleseedrenderer/projectbuilder.h"

// Build options header.
#include "foundation/core/buildoptions.h"

// appleseed.foundation headers.
#include "foundation/utility/autoreleaseptr.h"

// 3ds Max headers.
#include "appleseed-max-common/_beginmaxheaders.h"
#include <interactiverender.h>
#include <ISceneEventManager.h>
#include <maxapi.h>
#include "appleseed-max-common/_endmaxheaders.h"

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
{
  public:
    AppleseedInteractiveRender();
    ~AppleseedInteractiveRender() override;

    // IInteractiveRender methods.
    void BeginSession() override;
    void EndSession() override;
    void SetOwnerWnd(HWND owner_wnd) override;
    HWND GetOwnerWnd() const override;
    void SetIIRenderMgr(IIRenderMgr* iirender_mgr) override;
    IIRenderMgr* GetIIRenderMgr(IIRenderMgr* iirender_mgr) const override;
    void SetBitmap(Bitmap* bitmap) override;
    Bitmap* GetBitmap(Bitmap* bitmap) const override;
    void SetSceneINode(INode* scene_inode) override;
    INode* GetSceneINode() const override;
    void SetUseViewINode(bool use_view_inode) override;
    bool GetUseViewINode() const override;
    void SetViewINode(INode* view_inode) override;
    INode* GetViewINode() const override;
    void SetViewExp(ViewExp* view_exp) override;
    ViewExp* GetViewExp() const override;
    void SetRegion(const Box2& region) override;
    const Box2& GetRegion() const override;
    void SetDefaultLights(DefaultLight* def_lights, int num_def_lights) override;
    const DefaultLight* GetDefaultLights(int& num_def_lights) const override;
    void SetProgressCallback(IRenderProgressCallback* prog_cb) override;
    const IRenderProgressCallback* GetProgressCallback() const override;
    void Render(Bitmap* bitmap) override;
    ULONG GetNodeHandle(int x, int y) override;
    bool GetScreenBBox(Box2& s_bbox, INode* inode) override;
    ActionTableId GetActionTableId() override;
    ActionCallback* GetActionCallback() override;
    BOOL IsRendering() override;

    // IAbortableRenderer methods.
    void AbortRender() override;

    void update_camera_object(INode* camera);
    void add_material(Mtl* mtl, INode* node);
    void update_material(Mtl* material);
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
    ObjectMap                                       m_object_map;
    MaterialMap                                     m_material_map;

    foundation::auto_release_ptr<renderer::Project> prepare_project(
        const RendererSettings&     renderer_settings,
        const ViewParams&           view_params,
        INode*                      camera_node,
        const TimeValue             time);
};
