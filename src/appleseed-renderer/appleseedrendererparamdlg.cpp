
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
#include "appleseedrendererparamdlg.h"

// appleseed-max headers.
#include "main.h"
#include "resource.h"

// 3ds Max headers.
#include <3dsmaxdlport.h>

namespace
{
    // Forward declaration.
    INT_PTR CALLBACK dialog_proc(
        HWND    hWnd,
        UINT    uMsg,
        WPARAM  wParam,
        LPARAM  lParam);
}

struct AppleseedRendererParamDlg::Impl
{
    IRendParams*        m_rend_params;
    BOOL                m_in_progress;

    HWND                m_rollup_sampling;
    ICustEdit*          m_text_pixelsamples;
    ISpinnerControl*    m_spinner_pixelsamples;

    Impl(
        IRendParams*    rend_params,
        BOOL            in_progress)
      : m_rend_params(rend_params)
      , m_in_progress(in_progress)
    {
        m_rollup_sampling =
            m_rend_params->AddRollupPage(
                g_module,
                MAKEINTRESOURCE(IDD_FORMVIEW_RENDERERPARAMS),
                &dialog_proc,
                _T("Sampling"),
                reinterpret_cast<LPARAM>(this));
    }

    ~Impl()
    {
        m_rend_params->DeleteRollupPage(m_rollup_sampling);
    }

    void init_dialog(HWND hWnd)
    {
        m_text_pixelsamples = GetICustEdit(GetDlgItem(hWnd, IDC_TEXT_PIXELSAMPLES));
        m_spinner_pixelsamples = GetISpinner(GetDlgItem(hWnd, IDC_SPINNER_PIXELSAMPLES));
    }

    void destroy_dialog()
    {
        ReleaseICustEdit(m_text_pixelsamples);
        ReleaseISpinner(m_spinner_pixelsamples);
    }
};

namespace
{
    INT_PTR CALLBACK dialog_proc(
        HWND    hWnd,
        UINT    uMsg,
        WPARAM  wParam,
        LPARAM  lParam)
    {
        AppleseedRendererParamDlg::Impl* impl =
            uMsg == WM_INITDIALOG
                ? reinterpret_cast<AppleseedRendererParamDlg::Impl*>(lParam)
                : DLGetWindowLongPtr<AppleseedRendererParamDlg::Impl*>(hWnd);

        switch (uMsg)
        {
          case WM_INITDIALOG:
            DLSetWindowLongPtr(hWnd, impl);
            impl->init_dialog(hWnd);
            return TRUE;

          case WM_DESTROY:
            impl->destroy_dialog();
            return TRUE;
        }

        return FALSE;
    }
}

AppleseedRendererParamDlg::AppleseedRendererParamDlg(
    IRendParams*    rend_params,
    BOOL            in_progress)
  : impl(new Impl(rend_params, in_progress))
{
}

AppleseedRendererParamDlg::~AppleseedRendererParamDlg()
{
    delete impl;
}

void AppleseedRendererParamDlg::DeleteThis()
{
    delete this;
}

void AppleseedRendererParamDlg::AcceptParams()
{
}
