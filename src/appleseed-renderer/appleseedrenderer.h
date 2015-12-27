
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

#pragma once

// appleseed-max headers.
#include "maxsceneentities.h"
#include "renderersettings.h"

// appleseed.foundation headers.
#include "foundation/platform/windows.h"    // include before 3ds Max headers

// 3ds Max headers.
#include <iparamb2.h>
#include <max.h>
#include <render.h>
#undef base_type

// Windows headers.
#include <tchar.h>

// Standard headers.
#include <vector>

class AppleseedRenderer
  : public Renderer
{
  public:
    AppleseedRenderer();

    virtual Class_ID ClassID() override;

    virtual void GetClassName(MSTR& s) override;

    virtual void DeleteThis() override;

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

  private:
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
