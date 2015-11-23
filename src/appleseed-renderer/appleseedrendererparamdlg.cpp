
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
#include "renderersettings.h"
#include "resource.h"

// 3ds Max headers.
#include <3dsmaxdlport.h>

// Windows headers.
#include <tchar.h>

namespace
{
    // Forward declarations.
    INT_PTR CALLBACK sampling_rollup_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR CALLBACK system_rollup_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
}

struct AppleseedRendererParamDlg::Impl
{
    IRendParams*        m_rend_params;
    BOOL                m_in_progress;
    RendererSettings    m_settings;

    HWND                m_rollup_sampling;
    ICustEdit*          m_text_pixelsamples;
    ISpinnerControl*    m_spinner_pixelsamples;

    HWND                m_rollup_system;
    ICustEdit*          m_text_renderingthreads;
    ISpinnerControl*    m_spinner_renderingthreads;

    Impl(
        IRendParams*            rend_params,
        BOOL                    in_progress,
        const RendererSettings& settings)
      : m_rend_params(rend_params)
      , m_in_progress(in_progress)
      , m_settings(settings)
    {
        m_rollup_sampling =
            m_rend_params->AddRollupPage(
                g_module,
                MAKEINTRESOURCE(IDD_FORMVIEW_RENDERERPARAMS_SAMPLING),
                &sampling_rollup_proc,
                _T("Sampling"),
                reinterpret_cast<LPARAM>(this));

        m_rollup_system =
            m_rend_params->AddRollupPage(
                g_module,
                MAKEINTRESOURCE(IDD_FORMVIEW_RENDERERPARAMS_SYSTEM),
                &system_rollup_proc,
                _T("System"),
                reinterpret_cast<LPARAM>(this));
    }

    ~Impl()
    {
        destroy_system_controls();
        m_rend_params->DeleteRollupPage(m_rollup_system);

        destroy_sampling_controls();
        m_rend_params->DeleteRollupPage(m_rollup_sampling);
    }

    void create_sampling_controls(HWND hWnd)
    {
        m_text_pixelsamples = GetICustEdit(GetDlgItem(hWnd, IDC_TEXT_PIXELSAMPLES));

        m_spinner_pixelsamples = GetISpinner(GetDlgItem(hWnd, IDC_SPINNER_PIXELSAMPLES));
        m_spinner_pixelsamples->LinkToEdit(GetDlgItem(hWnd, IDC_TEXT_PIXELSAMPLES), EDITTYPE_INT);
        m_spinner_pixelsamples->SetLimits(1, 1000000, FALSE);
        m_spinner_pixelsamples->SetResetValue(static_cast<int>(RendererSettings::defaults().m_pixel_samples));
        m_spinner_pixelsamples->SetValue(static_cast<int>(m_settings.m_pixel_samples), FALSE);
    }

    void destroy_system_controls()
    {
        ReleaseISpinner(m_spinner_renderingthreads);
        ReleaseICustEdit(m_text_renderingthreads);
    }

    void create_system_controls(HWND hWnd)
    {
        m_text_renderingthreads = GetICustEdit(GetDlgItem(hWnd, IDC_TEXT_RENDERINGTHREADS));

        m_spinner_renderingthreads = GetISpinner(GetDlgItem(hWnd, IDC_SPINNER_RENDERINGTHREADS));
        m_spinner_renderingthreads->LinkToEdit(GetDlgItem(hWnd, IDC_TEXT_RENDERINGTHREADS), EDITTYPE_INT);
        m_spinner_renderingthreads->SetLimits(-1, 256, FALSE);
        m_spinner_renderingthreads->SetResetValue(static_cast<int>(RendererSettings::defaults().m_rendering_threads));
        m_spinner_renderingthreads->SetValue(static_cast<int>(m_settings.m_rendering_threads), FALSE);
    }

    void destroy_sampling_controls()
    {
        ReleaseISpinner(m_spinner_pixelsamples);
        ReleaseICustEdit(m_text_pixelsamples);
    }
};

namespace
{
    INT_PTR CALLBACK sampling_rollup_proc(
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
            impl->create_sampling_controls(hWnd);
            return TRUE;

          case WM_DESTROY:
            return TRUE;

          case WM_COMMAND:
            switch (LOWORD(wParam))
            {
              case IDC_TEXT_PIXELSAMPLES:
                impl->m_settings.m_pixel_samples = static_cast<size_t>(impl->m_text_pixelsamples->GetInt());
                return TRUE;
            }
            break;
        }

        return FALSE;
    }

    INT_PTR CALLBACK system_rollup_proc(
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
            impl->create_system_controls(hWnd);
            return TRUE;

          case WM_DESTROY:
            return TRUE;

          case WM_COMMAND:
            switch (LOWORD(wParam))
            {
              case IDC_TEXT_RENDERINGTHREADS:
                impl->m_settings.m_rendering_threads = static_cast<size_t>(impl->m_text_renderingthreads->GetInt());
                return TRUE;
            }
            break;
        }

        return FALSE;
    }
}

AppleseedRendererParamDlg::AppleseedRendererParamDlg(
    IRendParams*        rend_params,
    BOOL                in_progress,
    RendererSettings&   settings)
  : impl(new Impl(rend_params, in_progress, settings))
  , m_settings(settings)
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
    m_settings = impl->m_settings;
}
