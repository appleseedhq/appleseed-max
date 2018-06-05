
//
// This source file is part of appleseed.
// Visit https://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2015-2018 Francois Beaune, The appleseedhq Organization
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
#include "appleseedrenderer/appleseedrenderer.h"
#include "appleseedrenderer/renderersettings.h"
#include "appleseedrenderer/resource.h"
#include "appleseedrenderer/updatechecker.h"
#include "main.h"
#include "utilities.h"
#include "version.h"

// appleseed.foundation headers.
#include "foundation/core/appleseed.h"

// 3ds Max headers.
#include <3dsmaxdlport.h>
#include <assert1.h>
#include <iparamm2.h>

// Windows headers.
#include <shellapi.h>
#include <tchar.h>

// Standard headers.
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
        filter.Append(L"Project Files (*.appleseed)");
        filter.Append(L"*.appleseed");
        filter.Append(L"All Files (*.*)");
        filter.Append(L"*.*");

        MSTR initial_dir;
        return
            GetCOREInterface14()->DoMaxSaveAsDialog(
                parent_hwnd,
                L"Save Project As...",
                filepath,
                initial_dir,
                filter);
    }

    // ------------------------------------------------------------------------------------------------
    // Base class for panels (rollups).
    // ------------------------------------------------------------------------------------------------

    struct PanelBase
    {
        virtual ~PanelBase() {}

        virtual void init(HWND hwnd) = 0;

        virtual INT_PTR CALLBACK dialog_proc(
            HWND                hwnd,
            UINT                umsg,
            WPARAM              wparam,
            LPARAM              lparam) = 0;

        static INT_PTR CALLBACK dialog_proc_entry(
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

              default:
                {
                    PanelBase* panel = DLGetWindowLongPtr<PanelBase*>(hwnd);
                    return panel->dialog_proc(hwnd, umsg, wparam, lparam);
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
        struct UpdateCheckData
        {
            bool            m_update_available;
            std::wstring    m_version_string;
            std::wstring    m_download_url;
        };

        IRendParams*                    m_rend_params;
        HWND                            m_rollup;
        HWND                            m_button_download_hwnd;
        ICustButton*                    m_button_download;
        std::auto_ptr<UpdateCheckData>  m_update_data;

        enum { WM_UPDATE_CHECK_DATA = WM_USER + 101 };

        explicit AboutPanel(
            IRendParams*        rend_params)
          : m_rend_params(rend_params)
        {
            m_rollup =
                rend_params->AddRollupPage(
                    g_module,
                    MAKEINTRESOURCE(IDD_FORMVIEW_RENDERERPARAMS_ABOUT),
                    &dialog_proc_entry,
                    L"About",
                    reinterpret_cast<LPARAM>(this));
        }

        ~AboutPanel() override
        {
            ReleaseICustButton(m_button_download);
            m_rend_params->DeleteRollupPage(m_rollup);
        }

        static void async_update_check(HWND hwnd)
        {
            std::auto_ptr<UpdateCheckData> data(new UpdateCheckData());
            data->m_update_available = false;

            std::string version_string, publication_date, download_url;
            if (check_for_update(version_string, publication_date, download_url))
            {
                const std::wstring wide_version_string = utf8_to_wide(version_string);
                if (wide_version_string > std::wstring(PluginVersionString))
                {
                    data->m_update_available = true;
                    data->m_version_string = wide_version_string;
                    data->m_download_url = utf8_to_wide(download_url);
                }
            }

            PostMessage(hwnd, WM_UPDATE_CHECK_DATA, reinterpret_cast<WPARAM>(data.release()), 0);
        }

        void init(HWND hwnd) override
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

        INT_PTR CALLBACK dialog_proc(
            HWND                hwnd,
            UINT                umsg,
            WPARAM              wparam,
            LPARAM              lparam) override
        {
            switch (umsg)
            {
              case WM_UPDATE_CHECK_DATA:
                {
                    m_update_data.reset(reinterpret_cast<UpdateCheckData*>(wparam));
                    if (m_update_data->m_update_available)
                    {
                        std::wstringstream sstr;
                        sstr << "UPDATE: Version " << m_update_data->m_version_string << " available.";
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
                        DbgAssert(m_update_data.get());
                        DbgAssert(m_update_data->m_update_available);
                        ShellExecute(
                            hwnd,
                            L"open",
                            m_update_data->m_download_url.c_str(),
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
    // Output panel.
    // ------------------------------------------------------------------------------------------------

    class OutputParamMapDlgProc
      : public ParamMap2UserDlgProc
    {
      public:
        explicit OutputParamMapDlgProc(IParamBlock2* pblock)
          : m_pblock(pblock)
        {
        }

        void DeleteThis() override
        {
            ReleaseICustButton(m_button_browse);
            ReleaseICustEdit(m_text_project_filepath);
            delete this;
        }

        INT_PTR DlgProc(
            TimeValue   t,
            IParamMap2* map,
            HWND        hwnd,
            UINT        umsg,
            WPARAM      wparam,
            LPARAM      lparam) override
        {
            switch (umsg)
            {
              case WM_INITDIALOG:
                init(hwnd);
                enable_disable_controls();
                return TRUE;

              case WM_COMMAND:
                switch (LOWORD(wparam))
                {
                  case IDC_BUTTON_BROWSE:
                    {
                        MSTR filepath;
                        if (get_save_project_filepath(hwnd, filepath))
                        {
                            map->GetParamBlock()->SetValueByName(L"project_path", filepath, 0);
                            ICustEdit* project_path = GetICustEdit(GetDlgItem(hwnd, IDC_TEXT_PROJECT_FILEPATH));
                            project_path->SetText(filepath);
                            ReleaseICustEdit(project_path);
                        }
                        return TRUE;
                    }

                  case IDC_RADIO_RENDER:
                    enable_disable_controls();
                    return TRUE;

                  case IDC_RADIO_SAVEPROJECT:
                    enable_disable_controls();
                    return TRUE;

                  case IDC_RADIO_SAVEPROJECT_AND_RENDER:
                    enable_disable_controls();
                    return TRUE;
 
                  default:
                    return FALSE;
                }

              default:
                return FALSE;
            }
        }

        void init(HWND hwnd)
        {
            m_radio_render_only = GetDlgItem(hwnd, IDC_RADIO_RENDER);
            m_radio_save_only = GetDlgItem(hwnd, IDC_RADIO_SAVEPROJECT);
            m_radio_save_and_render = GetDlgItem(hwnd, IDC_RADIO_SAVEPROJECT_AND_RENDER);
            m_static_project_filepath = GetDlgItem(hwnd, IDC_STATIC_PROJECT_FILEPATH);
            m_button_browse = GetICustButton(GetDlgItem(hwnd, IDC_BUTTON_BROWSE));
            m_text_project_filepath = GetICustEdit(GetDlgItem(hwnd, IDC_TEXT_PROJECT_FILEPATH));
        }

        void enable_disable_controls()
        {
            DbgAssert(m_pblock != nullptr);
            
            int temp_use_max_procedural_maps;
            int output_mode;
            m_pblock->GetValueByName(L"use_max_procedural_maps", 0, temp_use_max_procedural_maps, FOREVER);
            m_pblock->GetValueByName(L"output_mode", 0, output_mode, FOREVER);
            const bool use_max_procedural_maps = temp_use_max_procedural_maps > 0;
            const bool save_project =
                output_mode == static_cast<int>(RendererSettings::OutputMode::SaveProjectOnly) ||
                output_mode == static_cast<int>(RendererSettings::OutputMode::SaveProjectAndRender);

            m_text_project_filepath->Enable(save_project && !use_max_procedural_maps);
            EnableWindow(m_radio_render_only, use_max_procedural_maps ? FALSE : TRUE);
            EnableWindow(m_radio_save_only, use_max_procedural_maps ? FALSE : TRUE);
            EnableWindow(m_radio_save_and_render, use_max_procedural_maps ? FALSE : TRUE);

            EnableWindow(m_static_project_filepath, save_project && !use_max_procedural_maps ? TRUE : FALSE);
            m_button_browse->Enable(save_project && !use_max_procedural_maps);
        }

      private:
        HWND            m_radio_render_only;
        HWND            m_radio_save_only;
        HWND            m_radio_save_and_render;
        HWND            m_static_project_filepath;
        ICustButton*    m_button_browse;
        ICustEdit*      m_text_project_filepath;
        IParamBlock2*   m_pblock;
    };

    // ------------------------------------------------------------------------------------------------
    // System panel.
    // ------------------------------------------------------------------------------------------------

    class SystemParamMapDlgProc
      : public ParamMap2UserDlgProc
    {
      public:
        SystemParamMapDlgProc(AppleseedRenderer* renderer)
          : m_renderer(renderer)
        {
        }

        void DeleteThis() override
        {
            delete this;
        }

        INT_PTR DlgProc(
            TimeValue   t,
            IParamMap2* map,
            HWND        hwnd,
            UINT        umsg,
            WPARAM      wparam,
            LPARAM      lparam) override
        {
            switch (umsg)
            {
              case WM_COMMAND:
                switch (LOWORD(wparam))
                {
                  case IDC_BUTTON_LOG:
                    m_renderer->show_last_session_log();

                  case IDC_CHECK_USE_MAX_PROCEDURAL_MAPS:
                    {
                        auto* dlg_proc = map->GetParamBlock()->GetMap(0)->GetUserDlgProc();
                        static_cast<OutputParamMapDlgProc*>(dlg_proc)->enable_disable_controls();
                    }
                    break;

                  default:
                    return FALSE;
                }

              default:
                return FALSE;
            }
        }

      private:
        AppleseedRenderer*      m_renderer;
    };
}

struct AppleseedRendererParamDlg::Impl
{
    RendererSettings&           m_settings;         // output (accepted) settings

    std::auto_ptr<AboutPanel>   m_about_panel;

    Impl(
        IRendParams*        rend_params,
        BOOL                in_progress,
        RendererSettings&   settings,
        AppleseedRenderer*  renderer)
      : m_settings(settings)
    {
        if (!in_progress)
        {
            m_about_panel.reset(new AboutPanel(rend_params));
        }
    }
};

AppleseedRendererParamDlg::AppleseedRendererParamDlg(
    IRendParams*            rend_params,
    BOOL                    in_progress,
    RendererSettings&       settings,
    AppleseedRenderer*      renderer)
  : impl(new Impl(rend_params, in_progress, settings, renderer))
  , m_pmap_output(nullptr)
  , m_pmap_image_sampling(nullptr)
  , m_pmap_lighting(nullptr)
  , m_pmap_system(nullptr)
{
   m_pmap_output = CreateRParamMap2(
        0,
        renderer->GetParamBlock(0),
        rend_params,
        g_module,
        MAKEINTRESOURCE(IDD_FORMVIEW_RENDERERPARAMS_OUTPUT),
        L"Output",
        0,
        new OutputParamMapDlgProc(renderer->GetParamBlock(0)));

   m_pmap_image_sampling = CreateRParamMap2(
        1,
        renderer->GetParamBlock(0),
        rend_params,
        g_module,
        MAKEINTRESOURCE(IDD_FORMVIEW_RENDERERPARAMS_IMAGESAMPLING),
        L"Image Sampling",
        0);

   m_pmap_lighting = CreateRParamMap2(
       2,
       renderer->GetParamBlock(0),
       rend_params,
       g_module,
       MAKEINTRESOURCE(IDD_FORMVIEW_RENDERERPARAMS_LIGHTING),
       L"Lighting",
       0); 
   
   m_pmap_system = CreateRParamMap2(
       3,
       renderer->GetParamBlock(0),
       rend_params,
       g_module,
       MAKEINTRESOURCE(IDD_FORMVIEW_RENDERERPARAMS_SYSTEM),
       L"System",
       0,
       new SystemParamMapDlgProc(renderer));
}

AppleseedRendererParamDlg::~AppleseedRendererParamDlg()
{
    delete impl;
}

void AppleseedRendererParamDlg::DeleteThis()
{
    if (m_pmap_system != nullptr)
        DestroyRParamMap2(m_pmap_system);

    if (m_pmap_lighting != nullptr)
        DestroyRParamMap2(m_pmap_lighting);
    
    if (m_pmap_image_sampling != nullptr)
        DestroyRParamMap2(m_pmap_image_sampling);
    
    if (m_pmap_output != nullptr)
        DestroyRParamMap2(m_pmap_output);

    m_pmap_system = nullptr;
    m_pmap_lighting = nullptr;
    m_pmap_image_sampling = nullptr;
    m_pmap_output = nullptr;

    delete this;
}

void AppleseedRendererParamDlg::AcceptParams()
{
}
