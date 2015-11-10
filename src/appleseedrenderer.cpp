
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
#include "appleseedrenderer.h"

// appleseed-max headers.
#include "appleseedrendererparamdlg.h"

//
// AppleseedRenderer class implementation.
//

namespace
{
    const Class_ID AppleseedRendererClassId(0x6170706c, 0x73656564);    // appl seed
    const TCHAR* AppleseedRendererClassName = _T("appleseed Renderer");
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
    delete this;
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
    INode*                  vnode,
    ViewParams*             viewPar,
    RendParams&             rp,
    HWND                    hwnd,
    DefaultLight*           defaultLights,
    int                     numDefLights,
    RendProgressCallback*   prog)
{
    return 1;
}

int AppleseedRenderer::Render(
    TimeValue               t,
    Bitmap*                 tobm,
    FrameRendParams&        frp,
    HWND                    hwnd,
    RendProgressCallback*   prog,
    ViewParams*             viewPar)
{
    return 1;
}

void AppleseedRenderer::Close(
    HWND                    hwnd,
    RendProgressCallback*   prog)
{
}

RendParamDlg* AppleseedRenderer::CreateParamDialog(
    IRendParams*            ir,
    BOOL                    prog)
{
    return new AppleseedRendererParamDlg();
}

void AppleseedRenderer::ResetParams()
{
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

const TCHAR* AppleseedRendererClassDesc::ClassName()
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

const TCHAR* AppleseedRendererClassDesc::Category()
{
    return _T("");
}
