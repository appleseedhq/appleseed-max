
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
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
#include "appleseedosltexture/oslshadermetadata.h"
#include "appleseedosltexture/templategenerator.h"
#include "bump/bumpparammapdlgproc.h"
#include "bump/resource.h"
#include "main.h"
#include "oslutils.h"
#include "utilities.h"

// 3ds Max headers.
#include <iparamm2.h>
#include <maxtypes.h>

namespace asf = foundation;
namespace asr = renderer;

OSLParamDlg::OSLParamDlg(
    HWND                    hwMtlEdit, 
    IMtlParams*             imp, 
    MtlBase*                map,
    const OSLShaderInfo*    shader_info)
    : m_hmedit(hwMtlEdit)
    , m_bump_pmap(nullptr)
    , m_pmap(nullptr)
    , m_imp(imp)
    , m_osl_plugin(map)
    , m_shader_info(shader_info)
    , m_col_1(10)
    , m_col_2(85)
    , m_col_3(122)
    , m_col_4(159)
    , m_label_width(76)
    , m_edit_width(25)
    , m_edit_height(10)
    , m_tex_width(84)
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
    int y_pos = 17;
    for (auto& page : pages)
    {
        int height = page.second.height + 10;
        page.second.y_pos = y_pos + 10;
        dialog_template.AddButton((LPCSTR)page.first.c_str(), WS_VISIBLE | BS_GROUPBOX | WS_CHILD | WS_GROUP, NULL, 4, y_pos, 209, height, 0);
        y_pos += height;
        y_pos += 4;
    }
}

void OSLParamDlg::add_ui_parameter(
    DialogTemplate&         dialog_template,
    const MaxParam&         max_param,
    PageGroupMap&           pages)
{
    int y_pos = 5;
    auto param_page = pages.find(max_param.page_name);
    if (param_page != pages.end())
        y_pos = param_page->second.y_pos;

    int ctrl_id = max_param.max_ctrl_id;
    const std::string param_label = max_param.max_label_str + ":";
    dialog_template.AddStatic((LPCSTR)param_label.c_str(), WS_VISIBLE, NULL, m_col_1, y_pos, m_label_width, m_edit_height, ctrl_id++);
    switch (max_param.param_type)
    {
      case MaxParam::Float:
      case MaxParam::IntNumber:
        dialog_template.AddComponent((LPCSTR)"CustEdit", (LPCSTR)"Edit", WS_VISIBLE | WS_TABSTOP, NULL, m_col_2, y_pos, m_edit_width, m_edit_height, ctrl_id++);
        dialog_template.AddComponent((LPCSTR)"SpinnerControl", (LPCSTR)"Spinner", WS_VISIBLE | WS_TABSTOP, NULL, m_col_2 + m_edit_width + 2, y_pos, 7, m_edit_height, ctrl_id++);
        break;
      case MaxParam::VectorParam:
      case MaxParam::NormalParam:
      case MaxParam::PointParam:
        dialog_template.AddComponent((LPCSTR)"CustEdit", (LPCSTR)"Edit", WS_VISIBLE | WS_TABSTOP, NULL, m_col_2, y_pos, m_edit_width, m_edit_height, ctrl_id++);
        dialog_template.AddComponent((LPCSTR)"SpinnerControl", (LPCSTR)"Spinner", WS_VISIBLE | WS_TABSTOP, NULL, m_col_2 + m_edit_width + 2, y_pos, 7, m_edit_height, ctrl_id++);
        dialog_template.AddComponent((LPCSTR)"CustEdit", (LPCSTR)"Edit", WS_VISIBLE | WS_TABSTOP, NULL, m_col_3, y_pos, m_edit_width, m_edit_height, ctrl_id++);
        dialog_template.AddComponent((LPCSTR)"SpinnerControl", (LPCSTR)"Spinner", WS_VISIBLE | WS_TABSTOP, NULL, m_col_3 + m_edit_width + 2, y_pos, 7, m_edit_height, ctrl_id++);
        dialog_template.AddComponent((LPCSTR)"CustEdit", (LPCSTR)"Edit", WS_VISIBLE | WS_TABSTOP, NULL, m_col_4, y_pos, m_edit_width, m_edit_height, ctrl_id++);
        dialog_template.AddComponent((LPCSTR)"SpinnerControl", (LPCSTR)"Spinner", WS_VISIBLE | WS_TABSTOP, NULL, m_col_4 + m_edit_width + 2, y_pos, 7, m_edit_height, ctrl_id++);
        break;
      case MaxParam::IntCheckbox:
        dialog_template.AddButton((LPCSTR)"", WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP, NULL, m_col_2, y_pos, 10, 10, ctrl_id++);
        break;
      case MaxParam::IntMapper:
      case MaxParam::StringPopup:
        dialog_template.AddComboBox((LPCSTR)"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP | WS_VISIBLE, NULL, m_col_2, y_pos, m_tex_width, m_edit_height, ctrl_id++);
        y_pos += 2;
        break;
      case MaxParam::Color:
        dialog_template.AddComponent((LPCSTR)"ColorSwatch", (LPCSTR)"Color", WS_VISIBLE | WS_TABSTOP, NULL, m_col_2, y_pos, m_edit_width, m_edit_height, ctrl_id++);
        break;
      case MaxParam::String:
        dialog_template.AddComponent((LPCSTR)"CustEdit", (LPCSTR)"Edit", WS_VISIBLE | WS_TABSTOP, NULL, m_col_2, y_pos, m_tex_width, m_edit_height, ctrl_id++);
        break;
      case MaxParam::Closure:
        dialog_template.AddComponent((LPCSTR)"CustButton", (LPCSTR)"", WS_VISIBLE, NULL, m_col_3, y_pos, m_tex_width, m_edit_height, ctrl_id++);
    }

    if (max_param.connectable && max_param.param_type != MaxParam::Closure)
    {
        Texmap* tex_map = nullptr;
        Interval iv;
        m_osl_plugin->GetParamBlock(0)->GetValue(max_param.max_param_id + 1, 0, tex_map, iv);
        
        const char* label = tex_map != nullptr ? "M" : "";

        if (max_param.is_vector())
            dialog_template.AddComponent((LPCSTR)"CustButton", (LPCSTR)label, WS_VISIBLE, NULL, m_col_4 + m_edit_width + 11, y_pos, m_edit_height, m_edit_height, ctrl_id);
        else
            dialog_template.AddComponent((LPCSTR)"CustButton", (LPCSTR)"", WS_VISIBLE, NULL, m_col_3, y_pos, m_tex_width, m_edit_height, ctrl_id);
    }

    y_pos += 12;
    if (param_page != pages.end())
        param_page->second.y_pos = y_pos;
}

void OSLParamDlg::create_dialog()
{
    PageGroupMap pages;

    for (auto& osl_param : m_shader_info->m_params)
    {
        if (osl_param.page != "" &&
            osl_param.page != "UV Coordinates" &&
            osl_param.widget != "null")
        {
            int param_height = osl_param.m_max_param.param_type == MaxParam::IntMapper ||
                osl_param.m_max_param.param_type == MaxParam::StringPopup ? 15 : 12;
            pages[osl_param.page].height += param_height;
        }
    }

    int dlg_height = 27;
    for (const auto& page : pages)
    {
        dlg_height += page.second.height;
        dlg_height += 14;
    }

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
        if (osl_param.widget != "null" &&
            osl_param.paramName != "in_uvCoord" &&
            osl_param.paramName != "in_uvFilterSize")
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
        0);

    auto tn_vec = m_shader_info->findParam("Tn");

    if (!m_shader_info->m_is_texture &&tn_vec != nullptr)
        m_bump_pmap = CreateMParamMap2(
            m_osl_plugin->GetParamBlock(0),
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
