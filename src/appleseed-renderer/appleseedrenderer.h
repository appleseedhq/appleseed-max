
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

#ifndef APPLESEEDRENDERER_H
#define APPLESEEDRENDERER_H

// appleseed-max headers.
#include "maxsceneentities.h"
#include "renderersettings.h"

// appleseed.foundation headers.
#include "foundation/platform/compiler.h"
#include "foundation/platform/windows.h"    // include before 3ds Max headers

// 3ds Max headers.
#include <iparamb2.h>
#include <max.h>
#undef base_type

// Windows headers.
#include <tchar.h>

class AppleseedRenderer
  : public Renderer
{
  public:
    AppleseedRenderer();

    virtual Class_ID ClassID() APPLESEED_OVERRIDE;

    virtual void GetClassName(MSTR& s) APPLESEED_OVERRIDE;

    virtual void DeleteThis() APPLESEED_OVERRIDE;

    virtual RefResult NotifyRefChanged(
        const Interval&         changeInt,
        RefTargetHandle         hTarget,
        PartID&                 partID,
        RefMessage              message,
        BOOL                    propagate) APPLESEED_OVERRIDE;

    virtual int Open(
        INode*                  scene,
        INode*                  view_node,
        ViewParams*             view_params,
        RendParams&             rp,
        HWND                    hwnd,
        DefaultLight*           default_lights,
        int                     default_light_count,
        RendProgressCallback*   progress_cb) APPLESEED_OVERRIDE;

    virtual int Render(
        TimeValue               time,
        Bitmap*                 bitmap,
        FrameRendParams&        frp,
        HWND                    hwnd,
        RendProgressCallback*   progress_cb,
        ViewParams*             view_params) APPLESEED_OVERRIDE;

    virtual void Close(
        HWND                    hwnd,
        RendProgressCallback*   progress_cb) APPLESEED_OVERRIDE;

    virtual RendParamDlg* CreateParamDialog(
        IRendParams*            rend_params,
        BOOL                    in_progress) APPLESEED_OVERRIDE;

    virtual void ResetParams() APPLESEED_OVERRIDE;

  private:
    RendererSettings    m_settings;
    INode*              m_scene;
    INode*              m_view_node;
    ViewParams          m_view_params;
    DefaultLight*       m_default_lights;
    int                 m_default_light_count;
    TimeValue           m_time;
    MaxSceneEntities    m_entities;

    void clear();
};


//
// AppleseedRenderer class descriptor.
//

class AppleseedRendererClassDesc
  : public ClassDesc2
{
  public:
    virtual int IsPublic() APPLESEED_OVERRIDE;
    virtual void* Create(BOOL loading) APPLESEED_OVERRIDE;
    virtual const TCHAR* ClassName() APPLESEED_OVERRIDE;
    virtual SClass_ID SuperClassID() APPLESEED_OVERRIDE;
    virtual Class_ID ClassID() APPLESEED_OVERRIDE;
    virtual const TCHAR* Category() APPLESEED_OVERRIDE;
    virtual const TCHAR* InternalName() APPLESEED_OVERRIDE;
};

#endif	// !APPLESEEDRENDERER_H
