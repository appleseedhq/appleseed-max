
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
#include "utilities.h"

// 3ds Max headers.
#include <3dsmaxdlport.h>

// Windows headers.
#include <tchar.h>

// Standard headers.
#include <cstring>
#include <memory>
#include <string>

using namespace std;

namespace
{
    // ------------------------------------------------------------------------------------------------
    // Utilities.
    // ------------------------------------------------------------------------------------------------

    bool get_save_project_filepath(HWND parentWnd, MSTR& filepath)
    {
        FilterList filter;
        filter.Append(_T("Project Files (*.appleseed)"));
        filter.Append(_T("*.appleseed"));
        filter.Append(_T("All Files (*.*)"));
        filter.Append(_T("*.*"));

        MSTR initial_dir;
        return GetCOREInterface14()->DoMaxSaveAsDialog(parentWnd, _T("Save Project As..."), filepath, initial_dir, filter);
    }

    // ------------------------------------------------------------------------------------------------
    // Base class for panels (rollups).
    // ------------------------------------------------------------------------------------------------

    struct PanelBase
    {
        IRendParams*            m_rend_params;
        RendererSettings&       m_settings;

        PanelBase(
            IRendParams*        rend_params,
            BOOL                in_progress,
            RendererSettings&   settings)
          : m_rend_params(rend_params)
          , m_settings(settings)
        {
        }

        virtual void init(HWND hWnd) = 0;

        virtual INT_PTR CALLBACK window_proc(
            HWND                hWnd,
            UINT                uMsg,
            WPARAM              wParam,
            LPARAM              lParam) = 0;

        static INT_PTR CALLBACK window_proc_entry(
            HWND                hWnd,
            UINT                uMsg,
            WPARAM              wParam,
            LPARAM              lParam)
        {
            switch (uMsg)
            {
                case WM_INITDIALOG:
                {
                    PanelBase* panel = reinterpret_cast<PanelBase*>(lParam);
                    DLSetWindowLongPtr(hWnd, panel);
                    panel->init(hWnd);
                    return TRUE;
                }

                case WM_DESTROY:
                    return TRUE;

                default:
                {
                    PanelBase* panel = DLGetWindowLongPtr<PanelBase*>(hWnd);
                    return panel->window_proc(hWnd, uMsg, wParam, lParam);
                }
            }
        }
    };

    // ------------------------------------------------------------------------------------------------
    // Image Sampling panel.
    // ------------------------------------------------------------------------------------------------

    struct ImageSamplingPanel
      : public PanelBase
    {
        HWND                    m_rollup;
        ICustEdit*              m_text_pixelsamples;
        ISpinnerControl*        m_spinner_pixelsamples;
        ICustEdit*              m_text_passes;
        ISpinnerControl*        m_spinner_passes;

        ImageSamplingPanel(
            IRendParams*        rend_params,
            BOOL                in_progress,
            RendererSettings&   settings)
          : PanelBase(rend_params, in_progress, settings)
        {
            m_rollup =
                rend_params->AddRollupPage(
                    g_module,
                    MAKEINTRESOURCE(IDD_FORMVIEW_RENDERERPARAMS_IMAGESAMPLING),
                    &window_proc_entry,
                    _T("Image Sampling"),
                    reinterpret_cast<LPARAM>(this));
        }

        ~ImageSamplingPanel()
        {
            ReleaseISpinner(m_spinner_passes);
            ReleaseICustEdit(m_text_passes);
            ReleaseISpinner(m_spinner_pixelsamples);
            ReleaseICustEdit(m_text_pixelsamples);
            m_rend_params->DeleteRollupPage(m_rollup);
        }

        virtual void init(HWND hWnd) APPLESEED_OVERRIDE
        {
            // Pixel Samples.
            m_text_pixelsamples = GetICustEdit(GetDlgItem(hWnd, IDC_TEXT_PIXELSAMPLES));
            m_spinner_pixelsamples = GetISpinner(GetDlgItem(hWnd, IDC_SPINNER_PIXELSAMPLES));
            m_spinner_pixelsamples->LinkToEdit(GetDlgItem(hWnd, IDC_TEXT_PIXELSAMPLES), EDITTYPE_INT);
            m_spinner_pixelsamples->SetLimits(1, 1000000, FALSE);
            m_spinner_pixelsamples->SetResetValue(static_cast<int>(RendererSettings::defaults().m_pixel_samples));
            m_spinner_pixelsamples->SetValue(static_cast<int>(m_settings.m_pixel_samples), FALSE);

            // Passes.
            m_text_passes = GetICustEdit(GetDlgItem(hWnd, IDC_TEXT_PASSES));
            m_spinner_passes = GetISpinner(GetDlgItem(hWnd, IDC_SPINNER_PASSES));
            m_spinner_passes->LinkToEdit(GetDlgItem(hWnd, IDC_TEXT_PASSES), EDITTYPE_INT);
            m_spinner_passes->SetLimits(1, 1000000, FALSE);
            m_spinner_passes->SetResetValue(static_cast<int>(RendererSettings::defaults().m_passes));
            m_spinner_passes->SetValue(static_cast<int>(m_settings.m_passes), FALSE);
        }

        virtual INT_PTR CALLBACK window_proc(
            HWND                hWnd,
            UINT                uMsg,
            WPARAM              wParam,
            LPARAM              lParam) APPLESEED_OVERRIDE
        {
            switch (uMsg)
            {
                case CC_SPINNER_CHANGE:
                    switch (LOWORD(wParam))
                    {
                        case IDC_SPINNER_PIXELSAMPLES:
                            m_settings.m_pixel_samples = static_cast<size_t>(m_spinner_pixelsamples->GetIVal());
                            return TRUE;

                        case IDC_SPINNER_PASSES:
                            m_settings.m_passes = static_cast<size_t>(m_spinner_passes->GetIVal());
                            return TRUE;

                        default:
                            return FALSE;
                    }

                default:
                    return FALSE;
            }
        }
    };

    // ------------------------------------------------------------------------------------------------
    // Output panel.
    // ------------------------------------------------------------------------------------------------

    struct OutputPanel
      : public PanelBase
    {
        HWND                    m_rollup;
        ICustEdit*              m_text_project_filepath;
        ICustButton*            m_button_browse;

        OutputPanel(
            IRendParams*        rend_params,
            BOOL                in_progress,
            RendererSettings&   settings)
          : PanelBase(rend_params, in_progress, settings)
        {
            m_rollup =
                rend_params->AddRollupPage(
                    g_module,
                    MAKEINTRESOURCE(IDD_FORMVIEW_RENDERERPARAMS_OUTPUT),
                    &window_proc_entry,
                    _T("Output"),
                    reinterpret_cast<LPARAM>(this));
        }

        ~OutputPanel()
        {
            ReleaseICustButton(m_button_browse);
            ReleaseICustEdit(m_text_project_filepath);
            m_rend_params->DeleteRollupPage(m_rollup);
        }

        virtual void init(HWND hWnd) APPLESEED_OVERRIDE
        {
            CheckRadioButton(hWnd, IDC_RADIO_RENDER, IDC_RADIO_SAVEPROJECT_AND_RENDER, IDC_RADIO_RENDER);

            m_text_project_filepath = GetICustEdit(GetDlgItem(hWnd, IDC_TEXT_PROJECT_FILEPATH));
            m_text_project_filepath->Disable();

            m_button_browse = GetICustButton(GetDlgItem(hWnd, IDC_BUTTON_BROWSE));
            m_button_browse->Disable();
        }

        virtual INT_PTR CALLBACK window_proc(
            HWND                hWnd,
            UINT                uMsg,
            WPARAM              wParam,
            LPARAM              lParam) APPLESEED_OVERRIDE
        {
            switch (uMsg)
            {
                case WM_CUSTEDIT_ENTER:
                    switch (LOWORD(wParam))
                    {
                        case IDC_TEXT_PROJECT_FILEPATH:
                        {
                            MSTR filepath;
                            m_text_project_filepath->GetText(filepath);
                            m_settings.m_project_file_path = utf8_encode(filepath);
                        }

                        default:
                            return FALSE;
                    }

                case WM_COMMAND:
                    switch (LOWORD(wParam))
                    {
                        case IDC_RADIO_RENDER:
                        {
                            if (IsDlgButtonChecked(hWnd, IDC_RADIO_RENDER) == BST_CHECKED)
                            {
                                m_settings.m_output_mode = RendererSettings::OutputModeRenderOnly;
                                m_text_project_filepath->Disable();
                                m_button_browse->Disable();
                            }
                            return TRUE;
                        }

                        case IDC_RADIO_SAVEPROJECT:
                        {
                            if (IsDlgButtonChecked(hWnd, IDC_RADIO_SAVEPROJECT) == BST_CHECKED)
                            {
                                m_settings.m_output_mode = RendererSettings::OutputModeSaveProjectOnly;
                                m_text_project_filepath->Enable();
                                m_button_browse->Enable();
                            }
                            return TRUE;
                        }

                        case IDC_RADIO_SAVEPROJECT_AND_RENDER:
                        {
                            if (IsDlgButtonChecked(hWnd, IDC_RADIO_SAVEPROJECT_AND_RENDER) == BST_CHECKED)
                            {
                                m_settings.m_output_mode = RendererSettings::OutputModeSaveProjectAndRender;
                                m_text_project_filepath->Enable();
                                m_button_browse->Enable();
                            }
                            return TRUE;
                        }

                        case IDC_BUTTON_BROWSE:
                        {
                            MSTR filepath;
                            if (get_save_project_filepath(hWnd, filepath))
                            {
                                m_settings.m_project_file_path = utf8_encode(filepath);
                                m_text_project_filepath->SetText(filepath);
                            }
                            return TRUE;
                        }

                        default:
                            return FALSE;
                    }

                default:
                    return FALSE;
            }
        }
    };

    // ------------------------------------------------------------------------------------------------
    // System panel.
    // ------------------------------------------------------------------------------------------------

    struct SystemPanel
      : public PanelBase
    {
        HWND                    m_rollup;
        ICustEdit*              m_text_renderingthreads;
        ISpinnerControl*        m_spinner_renderingthreads;

        SystemPanel(
            IRendParams*        rend_params,
            BOOL                in_progress,
            RendererSettings&   settings)
          : PanelBase(rend_params, in_progress, settings)
        {
            m_rollup =
                rend_params->AddRollupPage(
                    g_module,
                    MAKEINTRESOURCE(IDD_FORMVIEW_RENDERERPARAMS_SYSTEM),
                    &window_proc_entry,
                    _T("System"),
                    reinterpret_cast<LPARAM>(this));
        }

        ~SystemPanel()
        {
            ReleaseISpinner(m_spinner_renderingthreads);
            ReleaseICustEdit(m_text_renderingthreads);
            m_rend_params->DeleteRollupPage(m_rollup);
        }

        virtual void init(HWND hWnd) APPLESEED_OVERRIDE
        {
            m_text_renderingthreads = GetICustEdit(GetDlgItem(hWnd, IDC_TEXT_RENDERINGTHREADS));
            m_spinner_renderingthreads = GetISpinner(GetDlgItem(hWnd, IDC_SPINNER_RENDERINGTHREADS));
            m_spinner_renderingthreads->LinkToEdit(GetDlgItem(hWnd, IDC_TEXT_RENDERINGTHREADS), EDITTYPE_INT);
            m_spinner_renderingthreads->SetLimits(-1, 256, FALSE);
            m_spinner_renderingthreads->SetResetValue(static_cast<int>(RendererSettings::defaults().m_rendering_threads));
            m_spinner_renderingthreads->SetValue(static_cast<int>(m_settings.m_rendering_threads), FALSE);
        }

        virtual INT_PTR CALLBACK window_proc(
            HWND                hWnd,
            UINT                uMsg,
            WPARAM              wParam,
            LPARAM              lParam) APPLESEED_OVERRIDE
        {
            switch (uMsg)
            {
                case CC_SPINNER_CHANGE:
                    switch (LOWORD(wParam))
                    {
                        case IDC_SPINNER_RENDERINGTHREADS:
                            m_settings.m_rendering_threads = static_cast<size_t>(m_spinner_renderingthreads->GetIVal());
                            return TRUE;

                        default:
                            return FALSE;
                    }

                default:
                    return FALSE;
            }
        }
    };
}

struct AppleseedRendererParamDlg::Impl
{
    RendererSettings                m_temp_settings;    // settings the dialog will modify
    RendererSettings&               m_settings;         // output (accepted) settings

    auto_ptr<ImageSamplingPanel>    m_sampling_panel;
    auto_ptr<OutputPanel>           m_output_panel;
    auto_ptr<SystemPanel>           m_system_panel;

    Impl(
        IRendParams*        rend_params,
        BOOL                in_progress,
        RendererSettings&   settings)
      : m_temp_settings(settings)
      , m_settings(settings)
      , m_sampling_panel(new ImageSamplingPanel(rend_params, in_progress, m_temp_settings))
      , m_output_panel(new OutputPanel(rend_params, in_progress, m_temp_settings))
      , m_system_panel(new SystemPanel(rend_params, in_progress, m_temp_settings))
    {
    }
};

AppleseedRendererParamDlg::AppleseedRendererParamDlg(
    IRendParams*            rend_params,
    BOOL                    in_progress,
    RendererSettings&       settings)
  : impl(new Impl(rend_params, in_progress, settings))
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
    impl->m_settings = impl->m_temp_settings;
}
