
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

#pragma once

// Windows headers.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>    // include before 3ds Max headers

// appleseed-max headers.
#include "appleseedoslplugin/oslclassdesc.h"

// 3ds Max headers.
#include <imtl.h>

// Standard headers.
#include <map>

// Forward declarations.
class IParamMap2;
class DialogTemplate;
class MaxParam;
class MtlBase;
class OSLTexture;
class OSLPlugin;
class OSLShaderInfo;

struct PageGroup
{
    int y_pos;
    int height;
};

typedef std::map<std::string, PageGroup> PageGroupMap;

class OSLParamDlg : public ParamDlg
{
  public:
    OSLParamDlg(
        HWND                    hwMtlEdit, 
        IMtlParams*             imp, 
        MtlBase*                map,
        const OSLShaderInfo*    shader_info);

    Class_ID ClassID() override;
    ReferenceTarget* GetThing() override;
    void SetThing(ReferenceTarget *m) override;
    void DeleteThis() override;
    void SetTime(TimeValue t);
    void ReloadDialog(void);
    void ActivateDlg(BOOL onOff);

  private:
    void add_groupboxes(
        DialogTemplate&         dialog_template,
        PageGroupMap&           pages);

    void add_ui_parameter(
        DialogTemplate&         dialog_template,
        const MaxParam&         max_param,
        PageGroupMap&           pages);

    void create_dialog();

    IParamMap2*                 m_bump_pmap;
    Class_ID                    m_class_id;
    int                         m_col_1;
    int                         m_col_2;
    int                         m_col_3;
    int                         m_col_4;
    int                         m_edit_height;
    int                         m_edit_width;
    HWND  	                    m_hmedit;
    IMtlParams*                 m_imp;
    int                         m_label_width;
    MtlBase*                    m_osl_plugin;
    IParamMap2*                 m_pmap;
    const OSLShaderInfo*        m_shader_info;
    int                         m_tex_width;
    int                         m_y_pos;
};