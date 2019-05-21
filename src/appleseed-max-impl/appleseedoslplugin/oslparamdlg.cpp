
//
// This source file is part of appleseed.
// Visit https://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2018 Sergo Pogosyan, The appleseedhq Organization
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
#include "oslparamdlg.h"

// appleseed-max headers.
#include "appleseedoslplugin/oslshadermetadata.h"
#include "appleseedoslplugin/templategenerator.h"
#include "bump/bumpparammapdlgproc.h"
#include "bump/resource.h"
#include "main.h"
#include "oslutils.h"
#include "utilities.h"

// 3ds Max headers.
#include "_beginmaxheaders.h"
#include <iparamm2.h>
#include <maxtypes.h>
#include "_endmaxheaders.h"

namespace asf = foundation;
namespace asr = renderer;

namespace
{
    class OSLParamMapDlgProc
      : public ParamMap2UserDlgProc
    {
      public:
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
              case WM_INITDIALOG:
                {
                    ParamBlockDesc2* desc = map->GetDesc();
                    for (int i = 0, e = desc->Count(); i < e; ++i)
                    {
                        const ParamDef param_def = desc->GetParamDef(desc->IndextoID(i));
                        if (param_def.ctrl_type == TYPE_SPINNER)
                        {
                            const int* ctrl_ids = param_def.ctrl_IDs;
                            const int spinner_ctrl_id = ctrl_ids[param_def.ctrl_count - 1];

                            ISpinnerControl* ispinner = GetISpinner(GetDlgItem(hwnd, spinner_ctrl_id));
                            if (param_def.type == TYPE_INT)
                                ispinner->SetResetValue(param_def.def.i);
                            else if (param_def.type == TYPE_FLOAT)
                                ispinner->SetResetValue(param_def.def.f);

                            ReleaseISpinner(ispinner);
                        }
                    }
                }
                return TRUE;

              default:
                return FALSE;
            }
        }
    };

    void adjust_group_heights(PageGroup* page)
    {
        for (auto child : page->m_children)
            adjust_group_heights(child);

        page->m_height += 12;                                   // extra space for the controls
        if (page->m_parent != nullptr)
        {
            page->m_parent->m_height += page->m_height;
            page->m_parent->m_height += 3;                      // spacing between the groups
        }
    }

    void adjust_group_positions(PageGroup* page)
    {
        int y_pos = page->m_y + page->m_height;
        for (auto it = page->m_children.rbegin(); it != page->m_children.rend(); ++it)
        {
            PageGroup* child = *it;
            child->m_y = y_pos - child->m_height - 3;       // spacing between the groups
            y_pos = child->m_y;

            child->m_width = page->m_width - 8;
            child->m_x = page->m_x + 4;
        }

        for (auto child : page->m_children)
            adjust_group_positions(child);
    }

    int build_page_tree(const std::vector<OSLParamInfo>& shader_params, PageGroupMap& pages)
    {
        // Calculate height of each group based on controls.
        for (auto& param : shader_params)
        {
            if (param.m_page != "" &&
                param.m_widget != "null" &&
                !param.m_max_hidden_attr)
            {
                const int param_height = 
                    param.m_max_param.m_param_type == MaxParam::IntMapper ||
                    param.m_max_param.m_param_type == MaxParam::StringPopup ? 15 : 12;

                pages[param.m_page].m_height += param_height;
            }
        }
        
        // Build root page.
        PageGroup root_page;
        root_page.m_width = 217;
        for (auto& page : pages)
        {
            auto parent_name = page.first;
            const size_t dot_pos = parent_name.find_last_of('.');
            if (dot_pos == std::string::npos)
            {
                page.second.m_short_name = page.first;
                root_page.m_children.push_back(&page.second);
                page.second.m_parent = &root_page;
            }
            else
            {
                page.second.m_short_name = parent_name.substr(dot_pos + 1, parent_name.size() - 1);
                parent_name = parent_name.substr(0, dot_pos);

                if (pages.find(parent_name) != pages.end())
                {
                    pages[parent_name].m_children.push_back(&page.second);
                    page.second.m_parent = &pages[parent_name];
                }
            }
        }

        adjust_group_heights(&root_page);
        adjust_group_positions(&root_page);

        for (auto page : root_page.m_children)
            page->m_parent = nullptr;

        return root_page.m_height;
    }
}

OSLParamDlg::OSLParamDlg(
    HWND                    hw_mtl_edit,
    IMtlParams*             imp, 
    MtlBase*                map,
    const OSLShaderInfo*    shader_info)
  : m_hmedit(hw_mtl_edit)
  , m_bump_pmap(nullptr)
  , m_pmap(nullptr)
  , m_imp(imp)
  , m_osl_plugin(map)
  , m_shader_info(shader_info)
{
    m_class_id = m_osl_plugin->ClassID();
    create_dialog();
}

Class_ID OSLParamDlg::ClassID()
{
    return m_class_id;
}

ReferenceTarget* OSLParamDlg::GetThing()
{
    return m_osl_plugin;
}

void OSLParamDlg::SetThing(ReferenceTarget *m)
{
    assert(m->ClassID() == m_osl_plugin->ClassID());
    m_osl_plugin = static_cast<MtlBase*>(m);
    m_pmap->SetParamBlock(m_osl_plugin->GetParamBlock(0));
}

void OSLParamDlg::DeleteThis()
{
    if (m_pmap != nullptr)
        DestroyMParamMap2(m_pmap);
    
    if (m_bump_pmap != nullptr)
        DestroyMParamMap2(m_bump_pmap);

    m_pmap = nullptr;
    m_bump_pmap = nullptr;
    delete this;
}

void OSLParamDlg::SetTime(TimeValue t) {}
void OSLParamDlg::ReloadDialog(void) {}
void OSLParamDlg::ActivateDlg(BOOL onOff) {}

void OSLParamDlg::add_groupboxes(
    DialogTemplate&         dialog_template,
    PageGroupMap&           pages)
{
    for (auto& page : pages)
    {
        page.second.m_y += 12;      // offset from the top of the dialog
        dialog_template.AddButton(
            (LPCSTR)page.second.m_short_name.c_str(),
            WS_VISIBLE | BS_GROUPBOX | WS_CHILD | WS_GROUP,
            NULL,
            page.second.m_x,
            page.second.m_y,
            page.second.m_width,
            page.second.m_height,
            0);
        
        page.second.m_y += 10;      // offset to start placing controls
    }
}

void OSLParamDlg::add_ui_parameter(
    DialogTemplate&         dialog_template,
    const MaxParam&         max_param,
    PageGroupMap&           pages)
{
    const int Col2X = 85;
    const int Col3X = 122;
    const int Col4X = 159;
    const int LabelWidth = 76;
    const int EditWidth = 25;
    const int EditHeight = 10;
    const int TexButtonWidth = 84;

    int col1_x = 10;
    int y_pos = 5;
    auto param_page = pages.find(max_param.m_page_name);
    if (param_page != pages.end())
    {
        y_pos = param_page->second.m_y;
        col1_x = param_page->second.m_x + 5;
    }

    int ctrl_id = max_param.m_max_ctrl_id;
    const std::string param_label = max_param.m_max_label_str + ":";
    dialog_template.AddStatic((LPCSTR)param_label.c_str(), WS_VISIBLE, NULL, col1_x, y_pos, LabelWidth, EditHeight, ctrl_id++);
    
    if (max_param.m_has_constant)
    {
        switch (max_param.m_param_type)
        {
          case MaxParam::Float:
          case MaxParam::IntNumber:
            dialog_template.AddComponent((LPCSTR)"CustEdit", (LPCSTR)"Edit", WS_VISIBLE | WS_TABSTOP, NULL, Col2X, y_pos, EditWidth, EditHeight, ctrl_id++);
            dialog_template.AddComponent((LPCSTR)"SpinnerControl", (LPCSTR)"Spinner", WS_VISIBLE | WS_TABSTOP, NULL, Col2X + EditWidth + 2, y_pos, 7, EditHeight, ctrl_id++);
            break;

          case MaxParam::VectorParam:
          case MaxParam::NormalParam:
          case MaxParam::PointParam:
            dialog_template.AddComponent((LPCSTR)"CustEdit", (LPCSTR)"Edit", WS_VISIBLE | WS_TABSTOP, NULL, Col2X, y_pos, EditWidth, EditHeight, ctrl_id++);
            dialog_template.AddComponent((LPCSTR)"SpinnerControl", (LPCSTR)"Spinner", WS_VISIBLE | WS_TABSTOP, NULL, Col2X + EditWidth + 2, y_pos, 7, EditHeight, ctrl_id++);
            dialog_template.AddComponent((LPCSTR)"CustEdit", (LPCSTR)"Edit", WS_VISIBLE | WS_TABSTOP, NULL, Col3X, y_pos, EditWidth, EditHeight, ctrl_id++);
            dialog_template.AddComponent((LPCSTR)"SpinnerControl", (LPCSTR)"Spinner", WS_VISIBLE | WS_TABSTOP, NULL, Col3X + EditWidth + 2, y_pos, 7, EditHeight, ctrl_id++);
            dialog_template.AddComponent((LPCSTR)"CustEdit", (LPCSTR)"Edit", WS_VISIBLE | WS_TABSTOP, NULL, Col4X, y_pos, EditWidth, EditHeight, ctrl_id++);
            dialog_template.AddComponent((LPCSTR)"SpinnerControl", (LPCSTR)"Spinner", WS_VISIBLE | WS_TABSTOP, NULL, Col4X + EditWidth + 2, y_pos, 7, EditHeight, ctrl_id++);
            break;

          case MaxParam::IntCheckbox:
            dialog_template.AddButton((LPCSTR)"", WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP, NULL, Col2X, y_pos, 10, 10, ctrl_id++);
            break;

          case MaxParam::IntMapper:
          case MaxParam::StringPopup:
            dialog_template.AddComboBox((LPCSTR)"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP | WS_VISIBLE, NULL, Col2X, y_pos, TexButtonWidth, EditHeight, ctrl_id++);
            y_pos += 2;
            break;

          case MaxParam::Color:
            dialog_template.AddComponent((LPCSTR)"ColorSwatch", (LPCSTR)"Color", WS_VISIBLE | WS_TABSTOP, NULL, Col2X, y_pos, EditWidth, EditHeight, ctrl_id++);
            break;

          case MaxParam::String:
            dialog_template.AddComponent((LPCSTR)"CustEdit", (LPCSTR)"Edit", WS_VISIBLE | WS_TABSTOP, NULL, Col2X, y_pos, TexButtonWidth, EditHeight, ctrl_id++);
            break;
        }
    }

    if (max_param.m_connectable)
    {
        if (max_param.m_param_type == MaxParam::Closure)
        {
            dialog_template.AddComponent((LPCSTR)"CustButton", (LPCSTR)"", WS_VISIBLE, NULL, Col3X, y_pos, TexButtonWidth, EditHeight, ctrl_id);
        }
        else
        {
            const bool short_button = max_param.m_has_constant && 
                (max_param.m_param_type == MaxParam::VectorParam || 
                    max_param.m_param_type == MaxParam::NormalParam ||
                    max_param.m_param_type == MaxParam::PointParam ||
                    max_param.m_param_type == MaxParam::StringPopup ||
                    max_param.m_param_type == MaxParam::IntMapper);

            if (short_button)
            {
                Texmap* tex_map = nullptr;
                Interval iv;
                m_osl_plugin->GetParamBlock(0)->GetValue(max_param.m_max_param_id + 1, 0, tex_map, iv);

                const char* label = tex_map != nullptr ? "M" : "";

                dialog_template.AddComponent((LPCSTR)"CustButton", (LPCSTR)label, WS_VISIBLE, NULL, Col4X + EditWidth + 11, y_pos, EditHeight, EditHeight, ctrl_id);
            }
            else
                dialog_template.AddComponent((LPCSTR)"CustButton", (LPCSTR)"", WS_VISIBLE, NULL, Col3X, y_pos, TexButtonWidth, EditHeight, ctrl_id);
        }
    }

    y_pos += 12;
    if (param_page != pages.end())
        param_page->second.m_y = y_pos;
}

void OSLParamDlg::create_dialog()
{
    PageGroupMap pages;

    int dlg_height = build_page_tree(m_shader_info->m_params, pages);
    dlg_height += 12;

    DialogTemplate dialogTemplate(
        (LPCSTR)"OSL Texture",
        DS_SETFONT | WS_CHILD | WS_VISIBLE,
        0,
        0,
        217,
        dlg_height,
        (LPCSTR)"MS Sans Serif",
        8);

    add_groupboxes(dialogTemplate, pages);

    add_ui_parameter(dialogTemplate, m_shader_info->m_output_param, pages);

    for (auto& osl_param : m_shader_info->m_params)
    {
        if (osl_param.m_widget != "null" &&
            !osl_param.m_max_hidden_attr)
            add_ui_parameter(dialogTemplate, osl_param.m_max_param, pages);
    }

    std::wstring rollout_header(m_shader_info->m_max_shader_name + L" Parameters");
    
    m_pmap = CreateMParamMap2(
        m_osl_plugin->GetParamBlock(0),
        m_imp,
        g_module,
        m_hmedit,
        nullptr,
        nullptr,
        (DLGTEMPLATE*)dialogTemplate,
        rollout_header.c_str(),
        0,
        new OSLParamMapDlgProc());

    auto tn_vec = m_shader_info->find_param("Tn");
    auto bump_normal = m_shader_info->find_maya_attribute("normalCamera");

    if (!m_shader_info->m_is_texture &&
        (tn_vec != nullptr || bump_normal != nullptr))
    {
        m_bump_pmap = CreateMParamMap2(
            m_osl_plugin->GetParamBlock(1),
            m_imp,
            g_module,
            m_hmedit,
            nullptr,
            nullptr,
            MAKEINTRESOURCE(IDD_FORMVIEW_BUMP_PARAMS),
            L"Bump Parameters",
            0,
            new BumpParamMapDlgProc());
    }
}
