
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

// appleseed.foundation headers.
#include "foundation/platform/compiler.h"
#include "foundation/platform/windows.h"

// 3ds Max headers.
#include <max.h>

// Windows headers.
#include <tchar.h>

class AppleseedRenderer
  : public Renderer
{
  public:
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
        INode*                  vnode,
        ViewParams*             viewPar,
        RendParams&             rp,
        HWND                    hwnd,
        DefaultLight*           defaultLights,
        int                     numDefLights,
        RendProgressCallback*   prog) APPLESEED_OVERRIDE;

    virtual int Render(
        TimeValue               t,
        Bitmap*                 tobm,
        FrameRendParams&        frp,
        HWND                    hwnd,
        RendProgressCallback*   prog,
        ViewParams*             viewPar) APPLESEED_OVERRIDE;

    virtual void Close(
        HWND                    hwnd,
        RendProgressCallback*   prog) APPLESEED_OVERRIDE;

    virtual RendParamDlg* CreateParamDialog(
        IRendParams*            ir,
        BOOL                    prog) APPLESEED_OVERRIDE;

    virtual void ResetParams() APPLESEED_OVERRIDE;
};


//
// AppleseedRenderer class descriptor.
//

class AppleseedRendererClassDesc
  : public ClassDesc
{
  public:
    virtual int IsPublic() APPLESEED_OVERRIDE;
    virtual void* Create(BOOL loading) APPLESEED_OVERRIDE;
    virtual const TCHAR* ClassName() APPLESEED_OVERRIDE;
    virtual SClass_ID SuperClassID() APPLESEED_OVERRIDE;
    virtual Class_ID ClassID() APPLESEED_OVERRIDE;
    virtual const TCHAR* Category() APPLESEED_OVERRIDE;
};

#endif	// !APPLESEEDRENDERER_H
