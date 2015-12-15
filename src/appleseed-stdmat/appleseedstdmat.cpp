
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
#include "appleseedstdmat.h"

// appleseed-max headers.
#include "appleseedstdmatparamdlg.h"
#include "resource.h"

// 3ds Max headers.
#include <color.h>
#include <interval.h>
#include <iparamm2.h>
#include <strclass.h>

// Windows headers.
#include <tchar.h>

namespace
{
    const TCHAR* AppleseedStdMatClassName = _T("appleseed Standard Material");
}

AppleseedStdMatClassDesc g_appleseed_stdmat_classdesc;


//
// AppleseedStdMat class implementation.
//

namespace
{
    ParamBlockDesc2 g_block_desc(
        // --- Required arguments ---
        0,                                          // parameter block's ID
        _T("appleseedStdMatParams"),                // internal name
        0,                                          // ID of the localized name string
        &g_appleseed_stdmat_classdesc,              // class descriptor
        P_AUTO_CONSTRUCT + P_AUTO_UI,               // block flags

        // --- P_AUTO_CONSTRUCT arguments ---
        0,                                          // parameter block's reference number

        // --- P_AUTO_UI arguments ---
        IDD_FORMVIEW_PARAMS,                        // ID of the dialog template
        IDS_FORMVIEW_PARAMS_TITLE,                  // ID of the dialog's title string
        0,                                          // IParamMap2 creation/deletion flag mask
        0,                                          // rollup creation flag
        nullptr,                                    // user dialog procedure

        // --- Parameters specifications ---

        // --- The end ---
        p_end);
}

Class_ID AppleseedStdMat::get_class_id()
{
    return Class_ID(0x331b1ff7, 0x16381b67);
}

AppleseedStdMat::AppleseedStdMat()
{
    g_appleseed_stdmat_classdesc.MakeAutoParamBlocks(this);
}

RefResult AppleseedStdMat::NotifyRefChanged(
    const Interval&         changeInt,
    RefTargetHandle         hTarget,
    PartID&                 partID,
    RefMessage              message,
    BOOL                    propagate)
{
    return REF_SUCCEED;
}

void AppleseedStdMat::Update(TimeValue t, Interval& valid)
{
}

void AppleseedStdMat::Reset()
{
}

Interval AppleseedStdMat::Validity(TimeValue t)
{
    // todo: implement.
    return FOREVER;
}

ParamDlg* AppleseedStdMat::CreateParamDlg(HWND hwMtlEdit, IMtlParams* imp)
{
    return g_appleseed_stdmat_classdesc.CreateParamDlgs(hwMtlEdit, imp, this);
}

IOResult AppleseedStdMat::Save(ISave* isave)
{
    bool success = true;

    // todo: implement.

    return success ? IO_OK : IO_ERROR;
}

IOResult AppleseedStdMat::Load(ILoad* iload)
{
    IOResult result = IO_OK;

    // todo: implement.

    return result;
}

Color AppleseedStdMat::GetAmbient(int mtlNum, BOOL backFace)
{
    return Color(0.0f, 0.0f, 0.0f);
}

Color AppleseedStdMat::GetDiffuse(int mtlNum, BOOL backFace)
{
    return Color(0.0f, 0.0f, 0.0f);
}

Color AppleseedStdMat::GetSpecular(int mtlNum, BOOL backFace)
{
    return Color(0.0f, 0.0f, 0.0f);
}

float AppleseedStdMat::GetShininess(int mtlNum, BOOL backFace)
{
    return 0.0f;
}

float AppleseedStdMat::GetShinStr(int mtlNum, BOOL backFace)
{
    return 0.0f;
}

float AppleseedStdMat::GetXParency(int mtlNum, BOOL backFace)
{
    return 0.0f;
}

void AppleseedStdMat::SetAmbient(Color c, TimeValue t)
{
}

void AppleseedStdMat::SetDiffuse(Color c, TimeValue t)
{
}

void AppleseedStdMat::SetSpecular(Color c, TimeValue t)
{
}

void AppleseedStdMat::SetShininess(float v, TimeValue t)
{
}

void AppleseedStdMat::Shade(ShadeContext& sc)
{
}


//
// AppleseedStdmatBrowserEntryInfo class implementation.
//

const MCHAR* AppleseedStdmatBrowserEntryInfo::GetEntryName() const
{
    return AppleseedStdMatClassName;
}

const MCHAR* AppleseedStdmatBrowserEntryInfo::GetEntryCategory() const
{
    return _T("Materials\\appleseed");
}

Bitmap* AppleseedStdmatBrowserEntryInfo::GetEntryThumbnail() const
{
    // todo: implement.
    return nullptr;
}


//
// AppleseedStdMatClassDesc class implementation.
//

int AppleseedStdMatClassDesc::IsPublic()
{
    return TRUE;
}

void* AppleseedStdMatClassDesc::Create(BOOL loading)
{
    return new AppleseedStdMat();
}

const MCHAR* AppleseedStdMatClassDesc::ClassName()
{
    return AppleseedStdMatClassName;
}

SClass_ID AppleseedStdMatClassDesc::SuperClassID()
{
    return MATERIAL_CLASS_ID;
}

Class_ID AppleseedStdMatClassDesc::ClassID()
{
    return AppleseedStdMat::get_class_id();
}

const MCHAR* AppleseedStdMatClassDesc::Category()
{
    return _T("");
}

const MCHAR* AppleseedStdMatClassDesc::InternalName()
{
    // Parsable name used by MAXScript.
    return _T("appleseed_stdmat");
}

FPInterface* AppleseedStdMatClassDesc::GetInterface(Interface_ID id)
{
    if (id == IMATERIAL_BROWSER_ENTRY_INFO_INTERFACE)
        return &m_browser_entry_info;

    return ClassDesc2::GetInterface(id);
}
