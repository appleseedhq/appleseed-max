
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
#include "appleseedstdmtlparamdlg.h"

// appleseed-max headers.
#include "appleseedstdmtl.h"

AppleseedStdMtlParamDlg::AppleseedStdMtlParamDlg(
    AppleseedStdMtl*    mtl,
    HWND                hwMtlEdit,
    IMtlParams*         imp)
  : m_mtl(mtl)
{
}

Class_ID AppleseedStdMtlParamDlg::ClassID()
{
    return AppleseedStdMtl::get_class_id();
}

void AppleseedStdMtlParamDlg::SetThing(ReferenceTarget* m)
{
    m_mtl = static_cast<AppleseedStdMtl*>(m);
}

ReferenceTarget* AppleseedStdMtlParamDlg::GetThing()
{
    return m_mtl;
}

void AppleseedStdMtlParamDlg::SetTime(TimeValue t)
{
}

void AppleseedStdMtlParamDlg::ReloadDialog()
{
}

void AppleseedStdMtlParamDlg::DeleteThis()
{
    delete this;
}

void AppleseedStdMtlParamDlg::ActivateDlg(BOOL onOff)
{
}
