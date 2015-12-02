
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
#include "updatechecker.h"
#include "utilities.h"
#include "version.h"

// appleseed.foundation headers.
#include "foundation/core/appleseed.h"

// 3ds Max headers.
#include <3dsmaxdlport.h>

// Windows headers.
#include <tchar.h>

// Standard headers.
#include <cassert>
#include <cstring>
#include <future>
#include <memory>
#include <sstream>
#include <string>

namespace asf = foundation;

namespace
{
    // ------------------------------------------------------------------------------------------------
    // Utilities.
    // ------------------------------------------------------------------------------------------------

    void set_label_text(const HWND parent_hwnd, const int control_id, const wchar_t* text)
    {
        SetDlgItemText(parent_hwnd, control_id, text);

        const HWND control_hwnd = GetDlgItem(parent_hwnd, control_id);

        RECT rect;
        GetClientRect(control_hwnd, &rect);
        InvalidateRect(control_hwnd, &rect, TRUE);
        MapWindowPoints(control_hwnd, parent_hwnd, reinterpret_cast<POINT*>(&rect), 2);
        RedrawWindow(parent_hwnd, &rect, nullptr, RDW_ERASE | RDW_INVALIDATE);
    }

    bool get_save_project_filepath(HWND parent_hwnd, MSTR& filepath)
    {
        FilterList filter;
        filter.Append(_T("Project Files (*.appleseed)"));
        filter.Append(_T("*.appleseed"));
        filter.Append(_T("All Files (*.*)"));
        filter.Append(_T("*.*"));

        MSTR initial_dir;
        return
            GetCOREInterface14()->DoMaxSaveAsDialog(
                parent_hwnd,
                _T("Save Project As..."),
                filepath,
                initial_dir,
                filter);
    }

    // ------------------------------------------------------------------------------------------------
    // Base class for panels (rollups).
    // ------------------------------------------------------------------------------------------------

    struct PanelBase
    {
        virtual void init(HWND hwnd) = 0;

        virtual INT_PTR CALLBACK window_proc(
            HWND                hwnd,
            UINT                umsg,
            WPARAM              wparam,
            LPARAM              lparam) = 0;

        static INT_PTR CALLBACK window_proc_entry(
            HWND                hwnd,
            UINT                umsg,
            WPARAM              wparam,
            LPARAM              lparam)
        {
            switch (umsg)
            {
                case WM_INITDIALOG:
                {
                    PanelBase* panel = reinterpret_cast<PanelBase*>(lparam);
                    DLSetWindowLongPtr(hwnd, panel);
                    panel->init(hwnd);
                    return TRUE;
                }

                case WM_DESTROY:
                    return TRUE;

                default:
                {
                    PanelBase* panel = DLGetWindowLongPtr<PanelBase*>(hwnd);
                    return panel->window_proc(hwnd, umsg, wparam, lparam);
                }
            }
        }
    };

    // ------------------------------------------------------------------------------------------------
    // About panel.
    // ------------------------------------------------------------------------------------------------

    struct AboutPanel
      : public PanelBase
    {
        IRendParams*            m_rend_params;
        HWND                    m_rollup;
        HWND                    m_button_download_hwnd;
        ICustButton*            m_button_download;
        bool                    m_update_available;
        std::wstring            m_download_url;

        enum { WM_UPDATE_CHECK_DATA = WM_USER + 101 };

        explicit AboutPanel(
            IRendParams*        rend_params)
          : m_rend_params(rend_params)
        {
            m_rollup =
                rend_params->AddRollupPage(
                    g_module,
                    MAKEINTRESOURCE(IDD_FORMVIEW_RENDERERPARAMS_ABOUT),
                    &window_proc_entry,
                    _T("About"),
                    reinterpret_cast<LPARAM>(this));
        }

        ~AboutPanel()
        {
            ReleaseICustButton(m_button_download);
            m_rend_params->DeleteRollupPage(m_rollup);
        }

        struct UpdateCheckData
        {
            bool            m_update_available;
            std::wstring    m_version_string;
            std::wstring    m_download_url;
        };

        static void async_update_check(HWND hwnd)
        {
            std::auto_ptr<UpdateCheckData> data(new UpdateCheckData());
            data->m_update_available = false;

            std::string version_string, publication_date, download_url;
            if (check_for_update(version_string, publication_date, download_url))
            {
                const std::wstring wide_version_string = utf8_to_wide(version_string);
                if (wide_version_string != std::wstring(PluginVersionString))
                {
                    data->m_update_available = true;
                    data->m_version_string = wide_version_string;
                    data->m_download_url = utf8_to_wide(download_url);
                }
            }

            PostMessage(hwnd, WM_UPDATE_CHECK_DATA, reinterpret_cast<WPARAM>(data.release()), 0);
        }

        virtual void init(HWND hwnd) override
        {
            m_button_download_hwnd = GetDlgItem(hwnd, IDC_BUTTON_DOWNLOAD);
            m_button_download = GetICustButton(m_button_download_hwnd);
            ShowWindow(m_button_download_hwnd, SW_HIDE);

            // Display the version strings of the plugin and of appleseed itself.
            GetICustStatus(GetDlgItem(hwnd, IDC_TEXT_PLUGIN_VERSION))->SetText(PluginVersionString);
            GetICustStatus(GetDlgItem(hwnd, IDC_TEXT_APPLESEED_VERSION))->SetText(
                utf8_to_wide(asf::Appleseed::get_lib_version()).c_str());

            // Asynchronously check if an update is available.
            std::async(std::launch::async, async_update_check, hwnd);
        }

        virtual INT_PTR CALLBACK window_proc(
            HWND                hwnd,
            UINT                umsg,
            WPARAM              wparam,
            LPARAM              lparam) override
        {
            switch (umsg)
            {
                case WM_UPDATE_CHECK_DATA:
                {
                    std::auto_ptr<UpdateCheckData> data(reinterpret_cast<UpdateCheckData*>(wparam));
                    if (data->m_update_available)
                    {
                        std::wstringstream sstr;
                        sstr << "UPDATE: Version " << data->m_version_string << " available.";
                        set_label_text(hwnd, IDC_STATIC_NEW_VERSION, sstr.str().c_str());
                        ShowWindow(m_button_download_hwnd, SW_SHOW);
                    }
                    else
                    {
                        set_label_text(hwnd, IDC_STATIC_NEW_VERSION, L"The plugin is up-to-date.");
                    }
                    return TRUE;
                }

                case WM_COMMAND:
                    switch (LOWORD(wparam))
                    {
                        case IDC_BUTTON_DOWNLOAD:
                        {
                            assert(m_update_available);
                            ShellExecute(
                                hwnd,
                                _T("open"),
                                m_download_url.c_str(),
                                nullptr,            // application parameters
                                nullptr,            // working directory
                                SW_SHOWNORMAL);
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
    // Image Sampling panel.
    // ------------------------------------------------------------------------------------------------

    struct ImageSamplingPanel
      : public PanelBase
    {
        IRendParams*            m_rend_params;
        RendererSettings&       m_settings;
        HWND                    m_rollup;
        ICustEdit*              m_text_pixelsamples;
        ISpinnerControl*        m_spinner_pixelsamples;
        ICustEdit*              m_text_passes;
        ISpinnerControl*        m_spinner_passes;

        ImageSamplingPanel(
            IRendParams*        rend_params,
            RendererSettings&   settings)
          : m_rend_params(rend_params)
          , m_settings(settings)
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

        virtual void init(HWND hwnd) override
        {
            // Pixel Samples.
            m_text_pixelsamples = GetICustEdit(GetDlgItem(hwnd, IDC_TEXT_PIXELSAMPLES));
            m_spinner_pixelsamples = GetISpinner(GetDlgItem(hwnd, IDC_SPINNER_PIXELSAMPLES));
            m_spinner_pixelsamples->LinkToEdit(GetDlgItem(hwnd, IDC_TEXT_PIXELSAMPLES), EDITTYPE_INT);
            m_spinner_pixelsamples->SetLimits(1, 1000000, FALSE);
            m_spinner_pixelsamples->SetResetValue(RendererSettings::defaults().m_pixel_samples);
            m_spinner_pixelsamples->SetValue(m_settings.m_pixel_samples, FALSE);

            // Passes.
            m_text_passes = GetICustEdit(GetDlgItem(hwnd, IDC_TEXT_PASSES));
            m_spinner_passes = GetISpinner(GetDlgItem(hwnd, IDC_SPINNER_PASSES));
            m_spinner_passes->LinkToEdit(GetDlgItem(hwnd, IDC_TEXT_PASSES), EDITTYPE_INT);
            m_spinner_passes->SetLimits(1, 1000000, FALSE);
            m_spinner_passes->SetResetValue(RendererSettings::defaults().m_passes);
            m_spinner_passes->SetValue(m_settings.m_passes, FALSE);
        }

        virtual INT_PTR CALLBACK window_proc(
            HWND                hwnd,
            UINT                umsg,
            WPARAM              wparam,
            LPARAM              lparam) override
        {
            switch (umsg)
            {
                case CC_SPINNER_CHANGE:
                    switch (LOWORD(wparam))
                    {
                        case IDC_SPINNER_PIXELSAMPLES:
                            m_settings.m_pixel_samples = m_spinner_pixelsamples->GetIVal();
                            return TRUE;

                        case IDC_SPINNER_PASSES:
                            m_settings.m_passes = m_spinner_passes->GetIVal();
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
    // Lighting panel.
    // ------------------------------------------------------------------------------------------------

    struct LightingPanel
      : public PanelBase
    {
        IRendParams*            m_rend_params;
        RendererSettings&       m_settings;
        HWND                    m_rollup;
        ICustEdit*              m_text_bounces;
        ISpinnerControl*        m_spinner_bounces;

        LightingPanel(
            IRendParams*        rend_params,
            RendererSettings&   settings)
          : m_rend_params(rend_params)
          , m_settings(settings)
        {
            m_rollup =
                rend_params->AddRollupPage(
                    g_module,
                    MAKEINTRESOURCE(IDD_FORMVIEW_RENDERERPARAMS_LIGHTING),
                    &window_proc_entry,
                    _T("Lighting"),
                    reinterpret_cast<LPARAM>(this));
        }

        ~LightingPanel()
        {
            ReleaseISpinner(m_spinner_bounces);
            ReleaseICustEdit(m_text_bounces);
            m_rend_params->DeleteRollupPage(m_rollup);
        }

        virtual void init(HWND hwnd) override
        {
            m_text_bounces = GetICustEdit(GetDlgItem(hwnd, IDC_TEXT_BOUNCES));
            m_spinner_bounces = GetISpinner(GetDlgItem(hwnd, IDC_SPINNER_BOUNCES));
            m_spinner_bounces->LinkToEdit(GetDlgItem(hwnd, IDC_TEXT_BOUNCES), EDITTYPE_INT);
            m_spinner_bounces->SetLimits(0, 1000, FALSE);
            m_spinner_bounces->SetResetValue(RendererSettings::defaults().m_bounces);
            m_spinner_bounces->SetValue(m_settings.m_bounces, FALSE);

            CheckDlgButton(hwnd, IDC_CHECK_GI, m_settings.m_gi ? BST_CHECKED : BST_UNCHECKED);
            enable_disable_controls();
        }

        void enable_disable_controls()
        {
            m_text_bounces->Enable(m_settings.m_gi);
            m_spinner_bounces->Enable(m_settings.m_gi);
        }

        virtual INT_PTR CALLBACK window_proc(
            HWND                hwnd,
            UINT                umsg,
            WPARAM              wparam,
            LPARAM              lparam) override
        {
            switch (umsg)
            {
                case WM_COMMAND:
                    switch (LOWORD(wparam))
                    {
                        case IDC_CHECK_GI:
                            m_settings.m_gi = IsDlgButtonChecked(hwnd, IDC_CHECK_GI) == BST_CHECKED;
                            enable_disable_controls();
                            return TRUE;

                        default:
                            return FALSE;
                    }

                case CC_SPINNER_CHANGE:
                    switch (LOWORD(wparam))
                    {
                        case IDC_SPINNER_BOUNCES:
                            m_settings.m_bounces = m_spinner_bounces->GetIVal();
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
        IRendParams*            m_rend_params;
        RendererSettings&       m_settings;
        HWND                    m_rollup;
        ICustEdit*              m_text_project_filepath;
        ICustButton*            m_button_browse;

        OutputPanel(
            IRendParams*        rend_params,
            RendererSettings&   settings)
          : m_rend_params(rend_params)
          , m_settings(settings)
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

        virtual void init(HWND hwnd) override
        {
            m_text_project_filepath = GetICustEdit(GetDlgItem(hwnd, IDC_TEXT_PROJECT_FILEPATH));
            m_text_project_filepath->SetText(m_settings.m_project_file_path);
            m_button_browse = GetICustButton(GetDlgItem(hwnd, IDC_BUTTON_BROWSE));

            CheckRadioButton(
                hwnd,
                IDC_RADIO_RENDER,
                IDC_RADIO_SAVEPROJECT_AND_RENDER,
                m_settings.m_output_mode == RendererSettings::OutputMode::RenderOnly ? IDC_RADIO_RENDER :
                m_settings.m_output_mode == RendererSettings::OutputMode::SaveProjectOnly ? IDC_RADIO_SAVEPROJECT :
                IDC_RADIO_SAVEPROJECT_AND_RENDER);

            enable_disable_controls();
        }

        void enable_disable_controls()
        {
            const bool save_project =
                m_settings.m_output_mode == RendererSettings::OutputMode::SaveProjectOnly ||
                m_settings.m_output_mode == RendererSettings::OutputMode::SaveProjectAndRender;

            m_text_project_filepath->Enable(save_project);
            m_button_browse->Enable(save_project);
        }

        virtual INT_PTR CALLBACK window_proc(
            HWND                hwnd,
            UINT                umsg,
            WPARAM              wparam,
            LPARAM              lparam) override
        {
            switch (umsg)
            {
                case WM_CUSTEDIT_ENTER:
                    switch (LOWORD(wparam))
                    {
                        case IDC_TEXT_PROJECT_FILEPATH:
                            m_text_project_filepath->GetText(m_settings.m_project_file_path);
                            return TRUE;

                        default:
                            return FALSE;
                    }

                case WM_COMMAND:
                    switch (LOWORD(wparam))
                    {
                        case IDC_RADIO_RENDER:
                            m_settings.m_output_mode = RendererSettings::OutputMode::RenderOnly;
                            enable_disable_controls();
                            return TRUE;

                        case IDC_RADIO_SAVEPROJECT:
                            m_settings.m_output_mode = RendererSettings::OutputMode::SaveProjectOnly;
                            enable_disable_controls();
                            return TRUE;

                        case IDC_RADIO_SAVEPROJECT_AND_RENDER:
                            m_settings.m_output_mode = RendererSettings::OutputMode::SaveProjectAndRender;
                            enable_disable_controls();
                            return TRUE;

                        case IDC_BUTTON_BROWSE:
                        {
                            MSTR filepath;
                            if (get_save_project_filepath(hwnd, filepath))
                            {
                                m_settings.m_project_file_path = filepath;
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
        IRendParams*            m_rend_params;
        RendererSettings&       m_settings;
        HWND                    m_rollup;
        ICustEdit*              m_text_renderingthreads;
        ISpinnerControl*        m_spinner_renderingthreads;

        SystemPanel(
            IRendParams*        rend_params,
            RendererSettings&   settings)
          : m_rend_params(rend_params)
          , m_settings(settings)
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

        virtual void init(HWND hwnd) override
        {
            m_text_renderingthreads = GetICustEdit(GetDlgItem(hwnd, IDC_TEXT_RENDERINGTHREADS));
            m_spinner_renderingthreads = GetISpinner(GetDlgItem(hwnd, IDC_SPINNER_RENDERINGTHREADS));
            m_spinner_renderingthreads->LinkToEdit(GetDlgItem(hwnd, IDC_TEXT_RENDERINGTHREADS), EDITTYPE_INT);
            m_spinner_renderingthreads->SetLimits(-1, 256, FALSE);
            m_spinner_renderingthreads->SetResetValue(RendererSettings::defaults().m_rendering_threads);
            m_spinner_renderingthreads->SetValue(m_settings.m_rendering_threads, FALSE);
        }

        virtual INT_PTR CALLBACK window_proc(
            HWND                hwnd,
            UINT                umsg,
            WPARAM              wparam,
            LPARAM              lparam) override
        {
            switch (umsg)
            {
                case CC_SPINNER_CHANGE:
                    switch (LOWORD(wparam))
                    {
                        case IDC_SPINNER_RENDERINGTHREADS:
                            m_settings.m_rendering_threads = m_spinner_renderingthreads->GetIVal();
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
    RendererSettings                    m_temp_settings;    // settings the dialog will modify
    RendererSettings&                   m_settings;         // output (accepted) settings

    std::auto_ptr<AboutPanel>           m_about_panel;
    std::auto_ptr<ImageSamplingPanel>   m_image_sampling_panel;
    std::auto_ptr<LightingPanel>        m_lighting_panel;
    std::auto_ptr<OutputPanel>          m_output_panel;
    std::auto_ptr<SystemPanel>          m_system_panel;

    Impl(
        IRendParams*        rend_params,
        BOOL                in_progress,
        RendererSettings&   settings)
      : m_temp_settings(settings)
      , m_settings(settings)
    {
        if (!in_progress)
        {
            m_about_panel.reset(new AboutPanel(rend_params));
            m_image_sampling_panel.reset(new ImageSamplingPanel(rend_params, m_temp_settings));
            m_lighting_panel.reset(new LightingPanel(rend_params, m_temp_settings));
            m_output_panel.reset(new OutputPanel(rend_params, m_temp_settings));
            m_system_panel.reset(new SystemPanel(rend_params, m_temp_settings));
        }
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
